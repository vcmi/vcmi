"""Extraction of one release's section from ChangeLog.md."""

import re

from .errors import DataError


def extract_section(changelog, version):
    """Returns the section whose heading has the form '## <previous> -> <version>', including the heading."""
    lines = changelog.splitlines()
    heading = re.compile(r"^## .*->\s*(\S+)(.*)$")
    start = None
    for index, line in enumerate(lines):
        match = heading.match(line)
        if start is None:
            if match and match.group(1) == version:
                if match.group(2).strip():
                    raise DataError(f"changelog section for {version} is not final: '{line}'")
                start = index
        elif line.startswith("## "):
            return "\n".join(lines[start:index]).strip() + "\n"
    if start is None:
        raise DataError(f"changelog has no section for {version}")
    return "\n".join(lines[start:]).strip() + "\n"
