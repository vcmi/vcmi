"""Decision what a client is offered. Pure function of the request and a data snapshot."""

from dataclasses import dataclass

from .platforms import (ARCHITECTURES, CHANNELS, COMMIT_PATTERN, DISTRIBUTIONS, OPERATING_SYSTEMS, lower_action,
                        parse_version, platform_key)

COMPARE_URL = "https://github.com/vcmi/vcmi/compare/{old}...{new}"

# Highest action for each distribution that can receive a file of the same kind; all others get 'external'
DISTRIBUTION_ACTIONS = {
    "installer": "install",
    "package": "download",
}


@dataclass(frozen=True)
class Request:
    version: tuple
    os: str
    arch: str
    dist: str
    channel: str
    commit: str | None

    @property
    def key(self):
        return platform_key(self.os, self.arch)


def parse_request(params):
    """Returns Request, or None if parameters are missing or invalid. params maps names to single values."""
    version = parse_version(params.get("version"))
    os_name = params.get("os")
    arch = params.get("arch")
    dist = params.get("dist")
    channel = params.get("channel", "stable")
    commit = params.get("commit")
    if version is None or os_name not in OPERATING_SYSTEMS or arch not in ARCHITECTURES:
        return None
    if dist not in DISTRIBUTIONS or channel not in CHANNELS:
        return None
    if channel != "stable" and (commit is None or not COMMIT_PATTERN.fullmatch(commit)):
        return None
    return Request(version, os_name, arch, dist, channel, commit if channel != "stable" else None)


def _file_response(snapshot, branch, build_file):
    return {
        "url": f"{snapshot.download_url}/branch/{branch}/{build_file.path}",
        "sha256": build_file.sha256,
        "size": build_file.size,
    }


def _select_file(builds, dist, limit):
    """Returns (action, BuildFile or None) for a client of this distribution."""
    build_file = builds.get(dist)
    action = lower_action(DISTRIBUTION_ACTIONS.get(dist, "external"), limit)
    if build_file is None or action == "external":
        return "external", None
    return action, build_file


def _decide_release(request, snapshot, allow_same_version):
    newer = [offered for offered in snapshot.releases
             if offered.release.status == "published"
             and offered.release.applies_to(request.os, request.key)
             and (offered.release.version > request.version
                  or (allow_same_version and offered.release.version == request.version))]
    if not newer:
        return {"status": "none"}

    latest = newer[0]
    limit, notes = latest.release.merged(request.os, request.key)
    action, build_file = _select_file(latest.builds.get(request.key, {}), request.dist, limit)
    response = {
        "status": "update",
        "version": latest.release.version_text,
        "date": latest.release.date,
        "action": action,
    }
    if build_file is not None:
        response.update(_file_response(snapshot, "master", build_file))
    response["notes"] = notes
    response["changelog"] = "\n".join(offered.release.changelog for offered in newer)
    response["changelogUrl"] = snapshot.changelog_url
    return response


def _decide_branch(request, snapshot, branch):
    if request.dist not in DISTRIBUTION_ACTIONS:
        return {"status": "none"}
    index = snapshot.branches.get(branch)
    if index is None:
        return {"status": "none"}
    found = index.latest.get((request.key, request.dist))
    if found is None:
        return {"status": "none"}
    manifest, build_file = found
    client_run = index.runs.get(request.commit)
    if manifest.commit == request.commit or (client_run is not None and client_run >= manifest.run):
        return {"status": "none"}

    response = {
        "status": "update",
        "version": manifest.version,
        "date": manifest.date[:10],
        "action": DISTRIBUTION_ACTIONS[request.dist],
    }
    response.update(_file_response(snapshot, branch, build_file))
    response["notes"] = []
    response["changelog"] = ""
    response["changelogUrl"] = COMPARE_URL.format(old=request.commit, new=manifest.commit)
    return response


def decide(request, snapshot):
    """Returns the response for a Request from parse_request() or None, as a JSON-serializable dict."""
    if request is None:
        return {"status": "none"}
    channel = snapshot.channels.get(request.channel)
    if channel is None:
        return {"status": "none"}
    if channel.source == "branch":
        return _decide_branch(request, snapshot, channel.branch)
    return _decide_release(request, snapshot, allow_same_version=request.channel != "stable")
