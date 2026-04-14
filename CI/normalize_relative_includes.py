#!/usr/bin/env python3

"""Check local relative include directives in tracked C/C++ files."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

SOURCE_EXTENSIONS = {
    ".cpp",
    ".h",
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
LOCAL_INCLUDE_PREFIXES = ("./", "../")


def tracked_source_files(repo_root: Path) -> list[Path]:
    output = subprocess.check_output(
        ["git", "-C", str(repo_root), "ls-files"],
        text=True,
    )
    files: list[Path] = []
    for rel in output.splitlines():
        path = repo_root / rel
        if path.suffix.lower() not in SOURCE_EXTENSIONS:
            continue
        if path.is_file():
            files.append(path)
    return files


def canonical_include_path(include_path: str, file_path: Path, line_no: int) -> str | None:
    if not include_path.startswith(LOCAL_INCLUDE_PREFIXES):
        return None

    file_dir = file_path.parent
    target = (file_dir / include_path).resolve(strict=False)
    if not target.is_file():
        raise FileNotFoundError(
            f"{file_path}:{line_no}: include target does not exist: "
            f"{include_path} -> {target}"
        )

    canonical = os.path.relpath(target, start=file_dir).replace(os.sep, "/")
    if canonical == include_path:
        return None

    return canonical


def process_file(file_path: Path, repo_root: Path) -> list[str]:
    lines = file_path.read_text(encoding="utf-8").splitlines()
    issues: list[str] = []
    rel_path = file_path.relative_to(repo_root).as_posix()

    for line_no, line in enumerate(lines, start=1):
        match = INCLUDE_RE.match(line)
        if not match:
            continue

        include_path = match.group(1)
        canonical = canonical_include_path(include_path, file_path, line_no)
        if canonical is None:
            continue

        issues.append(
            f"{rel_path}:{line_no}: {include_path} -> {canonical}"
        )

    return issues


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check local relative include paths in tracked C/C++ files."
    )
    parser.add_argument(
        "--repo",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Repository root (defaults to script parent parent).",
    )
    args = parser.parse_args()

    repo_root = args.repo.resolve()
    files = tracked_source_files(repo_root)

    all_issues: list[str] = []
    try:
        for file_path in files:
            all_issues.extend(process_file(file_path, repo_root))
    except FileNotFoundError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    print(f"Files scanned: {len(files)}")
    print(f"Includes to normalize: {len(all_issues)}")

    for issue in all_issues:
        print(issue)

    if all_issues:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
