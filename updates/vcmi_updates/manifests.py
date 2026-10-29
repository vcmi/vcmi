"""Build manifests: one JSON per CI run, listing the builds that were uploaded to download.vcmi.eu."""

import pathlib
from dataclasses import dataclass

from .errors import DataError, check_fields
from .files import load_json
from .platforms import (BRANCH_PATTERN, COMMIT_PATTERN, FILE_KINDS, SHA256_PATTERN, is_platform_key,
                        parse_version)


@dataclass(frozen=True)
class BuildFile:
    path: str  # relative to branch/<branch>/ in the download tree
    sha256: str
    size: int


@dataclass(frozen=True)
class Manifest:
    branch: str
    commit: str
    run: int
    date: str
    version: str
    builds: dict  # platform key -> {kind: BuildFile}


def manifest_file_name(branch, commit):
    return f"VCMI-branch-{branch}-{commit}.json"


def manifest_path(download_root, branch, commit):
    return pathlib.Path(download_root) / "branch" / branch / "metadata" / manifest_file_name(branch, commit)


def _parse_file(data, source):
    check_fields(data, source, ("path", "sha256", "size"))
    if not isinstance(data["path"], str) or not data["path"]:
        raise DataError(f"{source}: 'path' must be a non-empty string")
    if not isinstance(data["sha256"], str) or not SHA256_PATTERN.fullmatch(data["sha256"]):
        raise DataError(f"{source}: 'sha256' must be 64 lowercase hex digits")
    if not isinstance(data["size"], int):
        raise DataError(f"{source}: 'size' must be an integer")
    return BuildFile(data["path"], data["sha256"], data["size"])


def parse_manifest(data, source):
    check_fields(data, source, ("branch", "commit", "run", "date", "version", "builds"))
    if not isinstance(data["branch"], str) or not BRANCH_PATTERN.fullmatch(data["branch"]):
        raise DataError(f"{source}: invalid 'branch'")
    if not isinstance(data["commit"], str) or not COMMIT_PATTERN.fullmatch(data["commit"]):
        raise DataError(f"{source}: 'commit' must be 7 lowercase hex digits")
    if not isinstance(data["run"], int):
        raise DataError(f"{source}: 'run' must be an integer")
    if not isinstance(data["date"], str) or not data["date"]:
        raise DataError(f"{source}: 'date' must be a non-empty string")
    if parse_version(data["version"]) is None:
        raise DataError(f"{source}: 'version' must have the form X.Y.Z")
    if not isinstance(data["builds"], dict):
        raise DataError(f"{source}: 'builds' must be an object")

    builds = {}
    for key, files in data["builds"].items():
        if not is_platform_key(key):
            raise DataError(f"{source}: unknown platform '{key}'")
        check_fields(files, f"{source}: {key}", (), FILE_KINDS)
        builds[key] = {kind: _parse_file(entry, f"{source}: {key}.{kind}") for kind, entry in files.items()}

    return Manifest(data["branch"], data["commit"], data["run"], data["date"], data["version"], builds)


def load_manifest(path, branch):
    """Loads a manifest and checks that it matches its location in the download tree."""
    path = pathlib.Path(path)
    source = str(path)
    manifest = parse_manifest(load_json(path), source)
    if manifest.branch != branch or path.name != manifest_file_name(manifest.branch, manifest.commit):
        raise DataError(f"{source}: branch or commit does not match the file location")
    return manifest
