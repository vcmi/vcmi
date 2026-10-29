"""AltStore source, /altstore.json: the template from the data directory with an entry for each published release."""

import copy

from .errors import DataError
from .files import load_json

PLATFORM = "ios-arm64"
KIND = "package"


def parse_template(data, source):
    """Checks the parts of the template that release entries are added to."""
    if not isinstance(data, dict) or not isinstance(data.get("apps"), list) or len(data["apps"]) != 1:
        raise DataError(f"{source}: expected an object with 'apps' holding one app")
    app = data["apps"][0]
    if not isinstance(app, dict) or not isinstance(app.get("versions"), list):
        raise DataError(f"{source}: the app must have a 'versions' list")
    for entry in app["versions"]:
        if not isinstance(entry, dict) or not isinstance(entry.get("version"), str):
            raise DataError(f"{source}: every entry of 'versions' must have a 'version' string")
    return data


def load_template(path):
    return parse_template(load_json(path), str(path))


def _description(notes, changelog):
    # Changelog sections start with a '## <previous> -> <version>' heading; AltStore shows the version already
    if changelog.startswith("## "):
        changelog = changelog.partition("\n")[2]
    return "\n\n".join([*notes, changelog.strip()])


def build_source(template, releases, branch_url, min_os_version, report):
    """Returns the template with entries of published iOS releases, highest first, before the entries it lists.

    releases are store.OfferedRelease, highest version first; branch_url is the download URL of their branch.
    """
    source = copy.deepcopy(template)
    app = source["apps"][0]
    listed = {entry["version"] for entry in app["versions"]}
    entries = []
    for offered in releases:
        release = offered.release
        if release.version_text in listed:
            raise DataError(f"AltStore template lists version {release.version_text}, which has a release record")
        if release.status != "published" or not release.applies_to("ios", PLATFORM):
            continue
        build_file = offered.builds.get(PLATFORM, {}).get(KIND)
        if build_file is None:
            report(f"{release.version_text}: no {PLATFORM} {KIND} that matches the build manifest, "
                   f"not listed in the AltStore source")
            continue
        _, notes = release.merged("ios", PLATFORM)
        entries.append({
            "version": release.version_text,
            "date": release.date,
            "size": build_file.size,
            "downloadURL": f"{branch_url}/{build_file.path}",
            "minOSVersion": min_os_version,
            "localizedDescription": _description(notes, release.changelog),
        })
    app["versions"] = entries + app["versions"]
    return source
