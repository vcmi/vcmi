"""Values accepted in requests, build manifests and release records."""

import re

OPERATING_SYSTEMS = ("windows", "macos", "linux", "android", "ios")
ARCHITECTURES = ("x86", "x64", "arm32", "arm64")
DISTRIBUTIONS = ("installer", "package", "googleplay", "fdroid", "flatpak", "testflight", "unknown")
CHANNELS = ("stable", "beta", "develop")

# Ordered from lowest to highest
ACTIONS = ("external", "download", "install")

# Kinds of files in a build manifest
FILE_KINDS = ("installer", "package")

# Used with fullmatch(); match() with '$' also accepts a trailing newline
VERSION_PATTERN = re.compile(r"([0-9]{1,6})\.([0-9]{1,6})\.([0-9]{1,6})")
COMMIT_PATTERN = re.compile(r"[0-9a-f]{7}")
BRANCH_PATTERN = re.compile(r"[A-Za-z0-9_]+")
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")


def parse_version(text):
    """Returns version as tuple of three integers, or None if text is not a valid version."""
    if not isinstance(text, str):
        return None
    match = VERSION_PATTERN.fullmatch(text)
    if not match:
        return None
    return tuple(int(part) for part in match.groups())


def platform_key(os_name, arch):
    return f"{os_name}-{arch}"


def is_platform_key(key):
    os_name, _, arch = key.partition("-")
    return os_name in OPERATING_SYSTEMS and arch in ARCHITECTURES


def is_section_key(key):
    """Release record sections are keyed either by OS or by platform key."""
    return key in OPERATING_SYSTEMS or is_platform_key(key)


def lower_action(first, second):
    return min(first, second, key=ACTIONS.index)
