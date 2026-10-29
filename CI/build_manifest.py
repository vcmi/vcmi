#!/usr/bin/env python3
"""Build manifest for the update service, see docs/maintainers/Update_Service.md.

  build_manifest.py file <uploaded file> <platform key> <installer|package>
      Run after a build is uploaded to download.vcmi.eu. Writes .update-info/<platform key>-<kind>.json.
  build_manifest.py create <directory with .update-info files> <upload name>.json
      Run once per CI run, after all uploads. Merges the files into the manifest of this run.
"""

import datetime
import hashlib
import json
import pathlib
import re
import sys

# Same naming as docs/maintainers/scripts/builds/sort_builds.sh, which moves uploads into place on the server
UPLOAD_NAME = re.compile(r"^(?P<run>[0-9]+)-vcmi-branch-(?P<branch>[^-]+)-(?P<commit>[a-f0-9]+)(-(?P<arch>[^.]+))?$")
SYSTEMS = {"exe": "windows", "zip": "windows-zip", "dmg": "macos", "ipa": "ios", "apk": "android", "AppImage": "linux"}
KINDS = ("installer", "package")
PLATFORM_KEY = re.compile(r"^(windows|macos|linux|android|ios)-(x86|x64|arm32|arm64)$")


def parse_upload_name(stem):
    match = UPLOAD_NAME.match(stem)
    if not match:
        raise ValueError(f"'{stem}' is not a branch build name")
    return match


def server_path(file_name):
    """Path relative to branch/<branch>/ on download.vcmi.eu that sort_builds.sh moves the upload to."""
    stem, _, extension = file_name.rpartition(".")
    match = parse_upload_name(stem)
    system = SYSTEMS[extension]
    folder = f"{system}-{match['arch']}" if match["arch"] else system
    return f"{folder}/VCMI-branch-{match['branch']}-{match['commit'][:7]}.{extension}"


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as file:
        while chunk := file.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()


def command_file(uploaded, key, kind):
    if not PLATFORM_KEY.match(key) or kind not in KINDS:
        raise ValueError(f"invalid platform key '{key}' or kind '{kind}'")
    uploaded = pathlib.Path(uploaded)
    info = {
        "platform": key,
        "kind": kind,
        "path": server_path(uploaded.name),
        "sha256": sha256(uploaded),
        "size": uploaded.stat().st_size,
    }
    output = pathlib.Path(".update-info") / f"{key}-{kind}.json"
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(info, indent=2) + "\n")
    print(f"Wrote {output}: {info}")


def read_version():
    text = pathlib.Path("cmake_modules/VersionDefinition.cmake").read_text()
    parts = [re.search(rf"set\(VCMI_VERSION_{part} (\d+)\)", text).group(1) for part in ("MAJOR", "MINOR", "PATCH")]
    return ".".join(parts)


def command_create(info_directory, output):
    builds = {}
    for path in sorted(pathlib.Path(info_directory).glob("*.json")):
        info = json.loads(path.read_text())
        files = builds.setdefault(info["platform"], {})
        if info["kind"] in files:
            raise ValueError(f"{path}: second {info['kind']} for {info['platform']}")
        files[info["kind"]] = {"path": info["path"], "sha256": info["sha256"], "size": info["size"]}
    if not builds:
        print("No builds were uploaded; no manifest created")
        return

    match = parse_upload_name(pathlib.Path(output).stem)
    manifest = {
        "branch": match["branch"],
        "commit": match["commit"][:7],
        "run": int(match["run"]),
        "date": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
        "version": read_version(),
        "builds": builds,
    }
    pathlib.Path(output).write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"Wrote {output}:")
    print(json.dumps(manifest, indent=2))


def main(argv):
    if len(argv) == 5 and argv[1] == "file":
        command_file(*argv[2:])
    elif len(argv) == 4 and argv[1] == "create":
        command_create(*argv[2:])
    else:
        print(__doc__, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
