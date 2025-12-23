#!/usr/bin/env python3

import json
import os
import pathlib
import re
import subprocess
import sys
from typing import Tuple, Optional, List


def run(cmd: List[str]) -> str:
    """Run a command and return stdout as text; return empty string on any error."""
    try:
        return subprocess.check_output(cmd, text=True, stderr=subprocess.STDOUT)
    except Exception:
        return ""


def detect(platform: str) -> Tuple[str, List[str], str]:
    """Detect cache tool, command to print stats, and a family label from platform."""
    if platform.startswith("msvc"):
        return ("sccache", ["sccache", "--show-stats"], "windows-msvc")
    if platform.startswith("mingw_"):
        return ("ccache", ["ccache", "-s"], "windows-mingw")
    if platform.startswith("mac"):
        return ("ccache", ["ccache", "-s"], "macos")
    if platform == "ios":
        return ("ccache", ["ccache", "-s"], "ios")
    if platform.startswith("android"):
        return ("ccache", ["ccache", "-s"], "android")
    return ("ccache", ["ccache", "-s"], "other")


def parse_ccache(text: str) -> Tuple[int, int]:
    """
    Parse ccache stats. Supports:
      - Legacy lines:  "Hits: 123" / "Misses: 45"
      - Modern lines:  "cache hit (direct)        10"
                       "cache hit (preprocessed)  5"
                       "cache hit (remote)        2"   (optional)
                       "cache miss                12"
    Returns (hits, misses).
    """
    # Legacy format
    m_hits = re.search(r"^\s*Hits:\s*(\d+)\b", text, re.M)
    m_miss = re.search(r"^\s*Misses:\s*(\d+)\b", text, re.M)
    if m_hits and m_miss:
        return int(m_hits.group(1)), int(m_miss.group(1))

    # Modern format: sum all hit buckets
    def pick(pattern: str) -> int:
        m = re.search(pattern, text, re.M | re.I)
        return int(m.group(1)) if m else 0

    hits_direct = pick(r"^cache hit\s*\(direct\)\s+(\d+)\b")
    hits_pre    = pick(r"^cache hit\s*\(preprocessed\)\s+(\d+)\b")
    hits_remote = pick(r"^cache hit\s*\(remote\)\s+(\d+)\b")  # may be absent
    misses      = pick(r"^cache miss\s+(\d+)\b")
    hits_total  = hits_direct + hits_pre + hits_remote
    return hits_total, misses


def parse_sccache(text: str) -> Tuple[int, int]:
    """
    Parse sccache --show-stats lines:
      "Cache hits            123"
      "Cache misses          45"
    Returns (hits, misses).
    """
    def pick(label: str) -> int:
        m = re.search(rf"^{re.escape(label)}\s+(\d+)\b", text, re.M | re.I)
        return int(m.group(1)) if m else 0

    hits = pick("Cache hits")
    misses = pick("Cache misses")
    return hits, misses


def arch_label(platform: str) -> str:
    """Produce a nice arch label."""
    mapping = {
        "mac-intel": "Intel",
        "mac-arm": "Apple Silicon",
        "ios": "ARM64",
        "msvc-x64": "x64",
        "msvc-x86": "x86",
        "msvc-arm64": "ARM64",
        "mingw_x86": "x86",
        "mingw_x86_64": "x64",
        "android-32": "ARMv7",
        "android-64": "ARM64",
        "android-64-intel": "x86_64",
    }
    return mapping.get(platform, platform)


def main() -> int:
    # Prefer our explicit PLATFORM env; fall back to VS's "Platform" on Windows if needed.
    platform = os.getenv("PLATFORM") or os.getenv("Platform") or "unknown"
    arch = arch_label(platform)
    tool, cmd, family = detect(platform)

    stats_raw = run(cmd)
    if tool == "sccache":
        hits, misses = parse_sccache(stats_raw)
    else:
        hits, misses = parse_ccache(stats_raw)

    total = hits + misses
    rate = f"{(100.0 * hits / total):.2f}%" if total else "n/a"

    payload = {
        "platform": platform,
        "family": family,
        "arch": arch,
        "tool": tool,
        "hits": hits,
        "misses": misses,
        "total": total,
        "rate": rate,
        "artifact_url": os.getenv("ARTIFACT_URL", ""),
        "debug_symbols_url": os.getenv("DEBUG_SYMBOLS_URL", ""),
        "aab_url": os.getenv("AAB_URL", ""),
        "stats_cmd": " ".join(cmd),
        "stats_raw": stats_raw,
    }

    outdir = pathlib.Path(".summary")
    outdir.mkdir(parents=True, exist_ok=True)
    outpath = outdir / f"{platform}.json"
    outpath.write_text(json.dumps(payload, ensure_ascii=False, indent=2))
    print(f"Wrote {outpath}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
