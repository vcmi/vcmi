"""vcmi-updates-release: creates and publishes release records on the server."""

import argparse
import json
import sys
import urllib.error
import urllib.request

from .changelog import extract_section
from .config import DEFAULT_PATH, load_config
from .errors import DataError
from .files import load_json, write_atomic
from .platforms import COMMIT_PATTERN, parse_version
from .records import load_release, parse_release, today
from .store import checked_builds, load_release_manifest, load_snapshot


def _load_checked_manifest(config, version, commit):
    """Loads the master manifest for commit; raises DataError unless it is for version and all its files match."""
    manifest = load_release_manifest(config.download_root, version, commit)
    problems = []
    checked_builds(manifest, config.download_root, True, problems.append)
    if problems:
        raise DataError("build files do not match the manifest:\n  " + "\n  ".join(problems))
    return manifest


def _read_changelog(config, version):
    try:
        with urllib.request.urlopen(config.changelog_source, timeout=30) as response:
            text = response.read().decode("utf-8")
    except urllib.error.URLError as error:
        raise DataError(f"cannot fetch {config.changelog_source}: {error}") from error
    return extract_section(text, version)


def command_add(config, args):
    if parse_version(args.version) is None:
        raise DataError(f"'{args.version}' does not have the form X.Y.Z")
    if not COMMIT_PATTERN.fullmatch(args.commit):
        raise DataError(f"'{args.commit}' is not a 7-digit lowercase commit hash")
    json_path = config.releases_dir / f"{args.version}.json"
    if json_path.exists():
        raise DataError(f"{json_path} already exists")

    manifest = _load_checked_manifest(config, args.version, args.commit)
    for key in args.only or []:
        if not any(platform == key or platform.startswith(key + "-") for platform in manifest.builds):
            raise DataError(f"build manifest has no builds for '{key}'")

    record = {
        "version": args.version,
        "commit": args.commit,
        "date": today(),
        "status": "draft",
    }
    record["common"] = {"notes": args.notes or []}
    if args.only:
        record["partial"] = True
        record["platforms"] = {key: {} for key in args.only}

    changelog = _read_changelog(config, args.version)
    text = json.dumps(record, indent="\t") + "\n"
    parse_release(record, str(json_path), args.version, changelog)

    # Changelog first: the service only picks up a record together with its changelog
    write_atomic(json_path.with_suffix(".md"), changelog)
    write_atomic(json_path, text)
    print("Platforms:", ", ".join(sorted(manifest.builds)))
    print(f"Created draft {json_path}")


def command_publish(config, args):
    json_path = config.releases_dir / f"{args.version}.json"
    release = load_release(json_path)
    if release.status != "draft":
        raise DataError(f"{args.version} has status '{release.status}', not 'draft'")
    _load_checked_manifest(config, release.version_text, release.commit)

    data = load_json(json_path)
    data["status"] = "published"
    data["date"] = today()
    write_atomic(json_path, json.dumps(data, indent="\t") + "\n")
    print(f"Published {args.version}")


def command_check(config, args):
    problems = []
    snapshot = load_snapshot(config, problems.append, full_check=True)
    for offered in snapshot.releases:
        print(f"{offered.release.version_text} {offered.release.status} ({offered.release.commit})")
    for channel in snapshot.channels.values():
        print(f"channel {channel.name}: {channel.source} {channel.branch or ''}".rstrip())
    if snapshot.altstore is None:
        problems.append(f"{config.altstore_template} does not exist; /altstore.json is not served")
    else:
        print(f"altstore: {len(snapshot.altstore['apps'][0]['versions'])} versions")
    if "stable" not in snapshot.channels:
        problems.append(f"{config.channels_dir / 'stable.json'} does not exist; no client is offered anything")
    for problem in problems:
        print(f"ERROR: {problem}")
    if problems:
        raise DataError(f"{len(problems)} problem(s) found")
    print("OK")


def main(argv=None):
    parser = argparse.ArgumentParser(prog="vcmi-updates-release", description="Manage VCMI release records")
    parser.add_argument("--config", default=DEFAULT_PATH)
    commands = parser.add_subparsers(dest="command", required=True)

    add = commands.add_parser("add", help="create a draft release record from a master build manifest")
    add.add_argument("version")
    add.add_argument("commit")
    add.add_argument("--notes", action="append", help="note shown to all players; may be repeated")
    add.add_argument("--only", nargs="+", metavar="KEY", help="partial release for these OS or platform keys")
    add.set_defaults(function=command_add)

    publish = commands.add_parser("publish", help="publish a draft release record")
    publish.add_argument("version")
    publish.set_defaults(function=command_publish)

    check = commands.add_parser("check", help="validate all data and build files")
    check.set_defaults(function=command_check)

    args = parser.parse_args(argv)
    config = load_config(args.config)
    try:
        args.function(config, args)
    except DataError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
