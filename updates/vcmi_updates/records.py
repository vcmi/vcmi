"""Release records: releases/<version>.json and the changelog section in releases/<version>.md."""

import datetime
import pathlib
from dataclasses import dataclass

from .errors import DataError, check_fields
from .files import load_json, read_text
from .platforms import ACTIONS, COMMIT_PATTERN, OPERATING_SYSTEMS, is_section_key, parse_version

STATUSES = ("draft", "published")


def today():
    """Current UTC date in the format of release dates."""
    return datetime.datetime.now(datetime.timezone.utc).date().isoformat()


@dataclass(frozen=True)
class Section:
    action: str | None  # highest action allowed for clients this section applies to
    notes: tuple


@dataclass(frozen=True)
class Release:
    version: tuple
    version_text: str
    commit: str
    date: str
    status: str
    partial: bool
    common: Section
    platforms: dict  # OS or platform key -> Section
    changelog: str

    def applies_to(self, os_name, key):
        return not self.partial or os_name in self.platforms or key in self.platforms

    def merged(self, os_name, key):
        """Returns (action, notes) for a client: common, then OS, then platform key."""
        action = "install"
        notes = []
        for section in (self.common, self.platforms.get(os_name), self.platforms.get(key)):
            if section is None:
                continue
            if section.action is not None:
                action = section.action
            notes.extend(section.notes)
        return action, notes


def _parse_section(data, source):
    check_fields(data, source, (), ("action", "notes"))
    action = data.get("action")
    if action is not None and action not in ACTIONS:
        raise DataError(f"{source}: 'action' must be one of {', '.join(ACTIONS)}")
    notes = data.get("notes", [])
    if not isinstance(notes, list) or not all(isinstance(note, str) and note for note in notes):
        raise DataError(f"{source}: 'notes' must be a list of non-empty strings")
    return Section(action, tuple(notes))


def parse_release(data, source, expected_version, changelog):
    check_fields(data, source, ("version", "commit", "date", "status"), ("partial", "common", "platforms"))
    version = parse_version(data["version"])
    if version is None or data["version"] != expected_version:
        raise DataError(f"{source}: 'version' must have the form X.Y.Z and match the file name")
    if not isinstance(data["commit"], str) or not COMMIT_PATTERN.fullmatch(data["commit"]):
        raise DataError(f"{source}: 'commit' must be 7 lowercase hex digits")
    try:
        datetime.date.fromisoformat(data["date"])
    except (TypeError, ValueError) as error:
        raise DataError(f"{source}: 'date' must have the form YYYY-MM-DD") from error
    if data["status"] not in STATUSES:
        raise DataError(f"{source}: 'status' must be one of {', '.join(STATUSES)}")
    partial = data.get("partial", False)
    if not isinstance(partial, bool):
        raise DataError(f"{source}: 'partial' must be true or false")

    common = _parse_section(data.get("common", {}), f"{source}: common")
    platforms_data = data.get("platforms", {})
    if not isinstance(platforms_data, dict):
        raise DataError(f"{source}: 'platforms' must be an object")
    platforms = {}
    for key, section in platforms_data.items():
        if not is_section_key(key):
            raise DataError(f"{source}: '{key}' is neither an OS ({', '.join(OPERATING_SYSTEMS)}) nor a platform key")
        platforms[key] = _parse_section(section, f"{source}: {key}")
    if partial and not platforms:
        raise DataError(f"{source}: a partial record must list its platforms")

    return Release(version, data["version"], data["commit"], data["date"], data["status"], partial, common,
                   platforms, changelog)


def load_release(json_path):
    json_path = pathlib.Path(json_path)
    changelog = read_text(json_path.with_suffix(".md"))
    return parse_release(load_json(json_path), str(json_path), json_path.stem, changelog)


def load_releases(releases_dir):
    """Returns all release records, highest version first."""
    releases = [load_release(path) for path in sorted(pathlib.Path(releases_dir).glob("*.json"))]
    releases.sort(key=lambda release: release.version, reverse=True)
    return releases
