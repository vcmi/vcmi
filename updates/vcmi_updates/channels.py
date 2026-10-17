"""Channel configurations: channels/<channel>.json."""

import pathlib
from dataclasses import dataclass

from .errors import DataError, check_fields
from .files import load_json
from .platforms import BRANCH_PATTERN, CHANNELS

SOURCES = ("releases", "branch")


@dataclass(frozen=True)
class Channel:
    name: str
    source: str
    branch: str | None  # only for source 'branch'


def parse_channel(data, name, source):
    if name not in CHANNELS:
        raise DataError(f"{source}: unknown channel '{name}'")
    check_fields(data, source, ("source",), ("branch",))
    if data["source"] == "releases":
        if "branch" in data:
            raise DataError(f"{source}: 'branch' is only allowed with source 'branch'")
        return Channel(name, "releases", None)
    if data["source"] == "branch":
        if name == "stable":
            raise DataError(f"{source}: channel 'stable' must use source 'releases'")
        branch = data.get("branch")
        if not isinstance(branch, str) or not BRANCH_PATTERN.fullmatch(branch):
            raise DataError(f"{source}: source 'branch' needs a valid 'branch'")
        return Channel(name, "branch", branch)
    raise DataError(f"{source}: 'source' must be one of {', '.join(SOURCES)}")


def load_channels(channels_dir):
    channels = {}
    for path in sorted(pathlib.Path(channels_dir).glob("*.json")):
        channels[path.stem] = parse_channel(load_json(path), path.stem, str(path))
    return channels
