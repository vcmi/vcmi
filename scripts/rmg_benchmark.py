#!/usr/bin/env python3
"""Quick-and-dirty RMG placement A/B harness. TEMPORARY debug tool.

Runs the vcmiserver --rmg-benchmark command once per zone-placement configuration,
each time rewriting config/randomMap.json, and prints how many connections ended up
as direct passages / subterranean gates / monoliths. Goal: verify the placement
changes actually reduce monolith fallbacks.

All configs share one random base seed (chosen per script run) so the comparison is
paired - same 20 maps, different placement algorithm - yet different every run.
"""

import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent          # .../vcmi/vcmi
CONFIG = REPO / "config" / "randomMap.json"
SERVER = Path.home() / "source" / "vcmi" / "gcc-release" / "bin" / "vcmiserver"

MAPS = int(os.environ.get("RMG_MAPS", "20"))
SEED = int(os.environ.get("RMG_SEED", str(int(time.time()) & 0x7FFFFFFF)))
SIZE = int(os.environ.get("RMG_SIZE", "144"))    # map width/height in tiles
LEVELS = int(os.environ.get("RMG_LEVELS", "2"))  # 1 = surface only (no cross-level connections)

# (label, hexGrid, saPolish, coupling, couplingStrength, crossAlignWeight, attempts) -- hub/robust dropped (shown inert)
# "new" = joint cross-level SA: discrete alignment (crossAlignWeight) does most cross-level work, so the
# continuous coupling can drop to 0.1 and stops tearing same-level grid neighbours apart.
CONFIGS = [
    ("original (all off)",       False, False, False, 0.0, 0.0, 1),
    ("prev (square coup, 8att)", False, False, True,  0.2, 0.0, 8),
    ("new (jointSA coup, 8att)",  True, True,  True,  0.1, 6.0, 8),
]

TEMPLATES = ["Mini Nostalgia", "Nostalgia", "6lm10a", "8xm8", "8xm12a", "Diamond"]


def set_config(text, hexgrid, sapolish, coupling, coupstrength, crossalign, attempts):
    text = re.sub(r'("crossLevelCoupling"\s*:\s*)(true|false)', r'\g<1>%s' % str(coupling).lower(), text)
    text = re.sub(r'("couplingStrength"\s*:\s*)[0-9.]+', r'\g<1>%s' % coupstrength, text)
    text = re.sub(r'("robustAttraction"\s*:\s*)(true|false)', r'\g<1>false', text)
    text = re.sub(r'("hexGrid"\s*:\s*)(true|false)', r'\g<1>%s' % str(hexgrid).lower(), text)
    text = re.sub(r'("hubFirst"\s*:\s*)(true|false)', r'\g<1>false', text)
    text = re.sub(r'("saPolish"\s*:\s*)(true|false)', r'\g<1>%s' % str(sapolish).lower(), text)
    text = re.sub(r'("crossAlignWeight"\s*:\s*)[0-9.]+', r'\g<1>%s' % crossalign, text)
    text = re.sub(r'("attempts"\s*:\s*)\d+', r'\g<1>%d' % attempts, text)
    return text


def run_once(template):
    proc = subprocess.run(
        [str(SERVER), "--rmg-benchmark", "--rmg-maps", str(MAPS), "--rmg-seed", str(SEED),
         "--rmg-size", str(SIZE), "--rmg-levels", str(LEVELS), "--rmg-template", template],
        cwd=str(SERVER.parent), capture_output=True, text=True, timeout=7200,
    )
    out = proc.stdout + proc.stderr
    for line in out.splitlines():
        idx = line.find("RMG_BENCH_SUMMARY")  # log color codes may prefix the line
        if idx >= 0:
            return dict(kv.split("=", 1) for kv in line[idx:].split()[1:])
    print("!! no summary line; last output:\n" + "\n".join(out.splitlines()[-15:]))
    return None


def render_table(template, results):
    # 'total' is every template connection; it stays constant across configs. The category
    # columns (direct/wide/water/gates/monol) plus intended-portal/virtual links (not shown)
    # sum to it - so the visible columns are not expected to add up to 'total' on their own.
    # Same-level monolith cause breakdown: naDist/naOrth/naDiag/naOth/noGuard/terr partition 'monol'
    # (on a single-level map, where there are no cross-level gate failures).
    # sizeMin/sizeMax: averaged smallest/largest actual-vs-expected zone-size ratio per map (1.0 = as intended).
    # A low sizeMin (zone starved) or high sizeMax (zone bloated) means placement distorted zone sizes.
    cols = ["total", "direct", "gates", "monol", "crossM", "naDist", "naOrth", "naDiag", "noGuard", "szMin", "szMax"]
    keys = ["connections", "direct", "gates", "monoliths", "crossMono", "naDist", "naOrth", "naDiag", "noGuard", "sizeMin", "sizeMax"]
    hdr = "%-26s | " % "config" + " | ".join("%6s" % c for c in cols) + " | %8s | %s" % ("monol/map", "vs base")
    lines = ["### template: %s" % template, "", hdr, "-" * len(hdr)]
    base_mono = None
    for label, r in results:
        if not r:
            lines.append("%-26s | (failed / not found)" % label)
            continue
        mono = int(r.get("monoliths", 0))
        if base_mono is None:
            base_mono = mono
        per_map = mono / MAPS if MAPS else 0
        delta = "-" if base_mono in (None, 0) else "%+.0f%%" % (100 * (mono - base_mono) / base_mono)
        vals = " | ".join("%6s" % r.get(k, "?") for k in keys)
        lines.append("%-26s | %s | %8.2f | %s" % (label, vals, per_map, delta))
    return "\n".join(lines)


def main():
    original = CONFIG.read_text()
    sections = ["RMG placement benchmark  (maps=%d, %dx%d, %d level(s), base seed=%d)"
                % (MAPS, SIZE, SIZE, LEVELS, SEED)]
    try:
        for template in TEMPLATES:
            results = []
            for label, hexgrid, sapolish, coupling, coupstrength, crossalign, attempts in CONFIGS:
                CONFIG.write_text(set_config(original, hexgrid, sapolish, coupling, coupstrength, crossalign, attempts))
                print("running: %-16s %-26s (hex=%d sa=%d coup=%s cross=%s att=%d) ..."
                      % (template, label, hexgrid, sapolish, coupstrength, crossalign, attempts), flush=True)
                results.append((label, run_once(template)))
            section = render_table(template, results)
            sections.append(section)
            print("\n" + section + "\n")
    finally:
        CONFIG.write_text(original)  # always restore

    report = "\n\n".join(sections)
    out_path = REPO / "rmg_benchmark_report.txt"
    out_path.write_text(report + "\n")
    print("\nsaved: %s" % out_path)


if __name__ == "__main__":
    if not SERVER.exists():
        sys.exit("vcmiserver not found at %s (build it first)" % SERVER)
    main()
