"""Loading of all data used by decide() into an immutable snapshot."""

import hashlib
import os
import pathlib
from dataclasses import dataclass

from .altstore import build_source, load_template
from .channels import load_channels
from .errors import DataError
from .manifests import load_manifest, manifest_path
from .records import load_releases

RELEASE_BRANCH = "master"


@dataclass(frozen=True)
class OfferedRelease:
    release: object  # records.Release
    builds: dict  # platform key -> {kind: BuildFile}; only files that passed the checks


@dataclass(frozen=True)
class BranchIndex:
    runs: dict  # commit -> run of its manifest
    latest: dict  # (platform key, kind) -> (Manifest, BuildFile) of the newest build that passed the checks


@dataclass(frozen=True)
class Snapshot:
    releases: list  # [OfferedRelease], highest version first
    channels: dict  # name -> Channel
    branches: dict  # branch -> BranchIndex
    download_url: str
    changelog_url: str
    altstore: dict | None  # AltStore source, None without a template


def file_problem(download_root, branch, build_file, check_hash):
    """Returns a description of what is wrong with the file, or None if it matches its manifest entry."""
    path = pathlib.Path(download_root) / "branch" / branch / build_file.path
    try:
        size = path.stat().st_size
        if size != build_file.size:
            return f"{path}: size is {size}, manifest has {build_file.size}"
        if check_hash and _sha256(path) != build_file.sha256:
            return f"{path}: sha256 does not match the manifest"
    except OSError as error:
        return f"{path}: {error.strerror}"
    return None


def _sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        while chunk := file.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def load_release_manifest(download_root, version, commit):
    """Loads the master manifest of a release; raises DataError if it is missing, invalid or for another version."""
    path = manifest_path(download_root, RELEASE_BRANCH, commit)
    manifest = load_manifest(path, RELEASE_BRANCH)
    if manifest.version != version:
        raise DataError(f"{version}: build manifest {path} is for version {manifest.version}")
    return manifest


def checked_builds(manifest, download_root, check_hash, report, superseded=lambda key: False):
    """Returns builds of the manifest whose files match it; problems with other files are passed to report().

    Hashes of files of superseded platforms are not checked.
    """
    builds = {}
    for key, files in manifest.builds.items():
        check_key_hash = check_hash and not superseded(key)
        for kind, build_file in files.items():
            problem = file_problem(download_root, manifest.branch, build_file, check_key_hash)
            if problem:
                report(problem)
            else:
                builds.setdefault(key, {})[kind] = build_file
    return builds


def _offered_release(release, config, report, full_check, superseded):
    if release.status != "published" and not (full_check and release.status == "draft"):
        return OfferedRelease(release, {})
    try:
        manifest = load_release_manifest(config.download_root, release.version_text, release.commit)
    except DataError as error:
        report(str(error))
        return OfferedRelease(release, {})
    return OfferedRelease(release, checked_builds(manifest, config.download_root, full_check, report, superseded))


def _branch_index(branch, config, report, full_check):
    manifests = []
    for path in (config.download_root / "branch" / branch / "metadata").glob("*.json"):
        try:
            manifests.append(load_manifest(path, branch))
        except DataError as error:
            report(str(error))
    manifests.sort(key=lambda manifest: manifest.run, reverse=True)

    latest = {}
    for manifest in manifests:
        for key, files in manifest.builds.items():
            for kind, build_file in files.items():
                if (key, kind) in latest:
                    continue
                problem = file_problem(config.download_root, branch, build_file, full_check)
                if problem:
                    report(problem)
                else:
                    latest[(key, kind)] = (manifest, build_file)
    return BranchIndex({manifest.commit: manifest.run for manifest in manifests}, latest)


def load_snapshot(config, report, full_check=False):
    """Loads all data. Raises DataError if records or channel configurations are invalid.

    Problems with build manifests and build files are passed to report(); the affected builds are not offered.
    The service checks only file sizes; full_check also checks drafts and file hashes.
    """
    releases = []
    published = []

    # Launchers only get files of the newest published release that applies to their platform; not hashing older
    # ones keeps full_check from hashing every release ever made. Sizes are still checked, the AltStore source
    # lists files of all releases
    def superseded(key):
        return any(newer.applies_to(key.partition("-")[0], key) for newer in published)

    for release in load_releases(config.releases_dir):
        releases.append(_offered_release(release, config, report, full_check, superseded))
        if release.status == "published":
            published.append(release)

    channels = load_channels(config.channels_dir)
    branches = {}
    for channel in channels.values():
        if channel.source == "branch" and channel.branch not in branches:
            branches[channel.branch] = _branch_index(channel.branch, config, report, full_check)

    altstore = None
    if config.altstore_template.exists():
        altstore = build_source(load_template(config.altstore_template), releases,
                                f"{config.download_url}/branch/{RELEASE_BRANCH}", config.altstore_min_os_version, report)
    return Snapshot(releases, channels, branches, config.download_url, config.changelog_url, altstore)


def data_signature(config):
    """Changes whenever any file that load_snapshot() reads from the data directory or metadata directories changes."""
    paths = [config.altstore_template, *config.releases_dir.glob("*"), *config.channels_dir.glob("*"),
             *config.download_root.glob("branch/*/metadata/*.json")]
    signature = []
    for path in sorted(paths):
        try:
            stat = os.stat(path)
        except OSError:
            continue
        signature.append((str(path), stat.st_mtime_ns, stat.st_size))
    return tuple(signature)
