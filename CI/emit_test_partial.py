#!/usr/bin/env python3

import json
import os
import pathlib
import sys

from emit_partial import parse_ccache, run


def main() -> int:
    platform = os.getenv("PLATFORM", "unknown")
    stats_raw = run(["ccache", "-s"])
    hits, misses = parse_ccache(stats_raw)
    total = hits + misses
    rate = f"{(100.0 * hits / total):.2f}%" if total else "n/a"

    payload = {
        "platform": platform,
        "hits": hits,
        "misses": misses,
        "total": total,
        "rate": rate,
        "stats_cmd": "ccache -s",
        "stats_raw": stats_raw,
    }

    outdir = pathlib.Path(".summary")
    outdir.mkdir(parents=True, exist_ok=True)
    outpath = outdir / f"test-{platform}.json"
    outpath.write_text(json.dumps(payload, ensure_ascii=False, indent=2))
    print(f"Wrote {outpath}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
