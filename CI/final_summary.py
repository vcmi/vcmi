#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import glob
import pathlib
import urllib.error
import urllib.parse
import urllib.request
import datetime
from typing import Any, Dict, Iterable, List, Tuple

# ----------------------- Constants -----------------------

ICON_WIN = "https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/32x32/WIN.png"
ICON_MAC = "https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/32x32/MAC.png"
ICON_IOS = "https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/32x32/IOS.png"
ICON_AND = "https://raw.githubusercontent.com/EgoistDeveloper/operating-system-logos/master/src/32x32/AND.png"
ICON_CPP = "https://raw.githubusercontent.com/isocpp/logos/master/cpp_logo.png"
ICON_PM  = "https://avatars.githubusercontent.com/u/96267164?s=32"

ALIGN_4_COLS = "|:--|:--:|:--:|:--:|\n"  # reused in Validation/Tests/Build matrix sections

FAMILIES = ("windows-msvc", "windows-mingw", "macos", "ios", "android")

VALIDATION_ORDER = {"LF line endings": 0, "JSON": 1, "Markdown": 2}
TESTS_ORDER = {"Clang Latest": 0, "GCC Latest": 1, "Clang Oldest": 2, "GCC Oldest": 3}

# ----------------------- Helpers -----------------------

def env(name: str, default: str = "") -> str:
    v = os.getenv(name)
    return v if v is not None else default

def now_utc() -> datetime.datetime:
    # Sonar: avoid utcnow(); use tz-aware now()
    return datetime.datetime.now(datetime.timezone.utc)

def hms_from_ms(ms: int) -> str:
    s = max(0, round(ms / 1000))
    hh = f"{s // 3600:02d}"
    mm = f"{(s % 3600) // 60:02d}"
    ss = f"{s % 60:02d}"
    return f"{hh}:{mm}:{ss}"

def parse_iso8601(s: str | None) -> datetime.datetime | None:
    if not s:
        return None
    try:
        return datetime.datetime.fromisoformat(s.replace("Z", "+00:00"))
    except Exception:
        return None

def gh_api(path: str, method: str = "GET", data: Any = None, token: str | None = None,
           params: Dict[str, Any] | None = None) -> Any:
    base = "https://api.github.com"
    url = f"{base}{path}"
    if params:
        url = f"{url}?{urllib.parse.urlencode(params)}"

    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "final-summary-script",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"

    req = urllib.request.Request(url, method=method, headers=headers)
    payload = None
    if data is not None:
        payload = json.dumps(data).encode("utf-8")
        req.add_header("Content-Type", "application/json")

    try:
        with urllib.request.urlopen(req, payload, timeout=60) as r:
            raw = r.read()
            return {} if not raw else json.loads(raw.decode("utf-8"))
    except urllib.error.HTTPError as e:
        msg = e.read().decode("utf-8", errors="replace")
        print(f"[WARN] GitHub API {method} {url} -> {e.code}: {msg}")
        raise
    except Exception as e:
        print(f"[WARN] GitHub API {method} {url} -> {e}")
        raise

def read_json_file(p: str) -> Any:
    try:
        with open(p, "r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        return None
    except Exception as e:
        print(f"[WARN] Cannot read JSON '{p}': {e}")
        return None

def write_json_file(p: str, data: Any) -> None:
    pathlib.Path(p).parent.mkdir(parents=True, exist_ok=True)
    with open(p, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

def append_summary(md: str) -> None:
    summary_path = env("GITHUB_STEP_SUMMARY") or "SUMMARY.md"
    with open(summary_path, "a", encoding="utf-8") as f:
        f.write(md)

def status_icon(status: str) -> str:
    return {
        "success": "✅",
        "failure": "❌",
        "cancelled": "🚫",
        "timed_out": "⌛",
        "skipped": "⏭",
        "neutral": "⚠️",
        "action_required": "⚠️",
    }.get(status, "❓")

def family_title_and_icon(fam: str) -> Tuple[str, str]:
    match fam:
        case "windows-msvc":  return "Windows (MSVC)", ICON_WIN
        case "windows-mingw": return "Windows (MinGW)", ICON_WIN
        case "macos":         return "macOS", ICON_MAC
        case "ios":           return "iOS", ICON_IOS
        case "android":       return "Android", ICON_AND
        case _:               return fam, ICON_PM

# ----------------------- 1) Collect validation & tests -----------------------

def _job_duration(job: Dict[str, Any]) -> str:
    st = parse_iso8601(job.get("started_at"))
    en = parse_iso8601(job.get("completed_at"))
    if st and en:
        return hms_from_ms(int((en - st).total_seconds() * 1000))
    return ""

def _test_pretty_name(name: str) -> str:
    pretty = name.replace("Test (", "").removesuffix(")")
    low = pretty.lower()
    if "gcc-latest" in low:   return "GCC Latest"
    if "gcc-oldest" in low:   return "GCC Oldest"
    if "clang-latest" in low: return "Clang Latest"
    if "clang-oldest" in low: return "Clang Oldest"
    return pretty

def _rows_for_job(j: Dict[str, Any]) -> List[Dict[str, Any]]:
    rows: List[Dict[str, Any]] = []
    dur = _job_duration(j)
    name = j.get("name") or ""

    # Build matrix
    if name.startswith("Build "):
        pretty = name.replace("Build (", "").removesuffix(")")
        rows.append({
            "group": "builds",
            "name": pretty,
            "status": j.get("conclusion") or "neutral",
            "duration": dur,
            "url": j.get("html_url"),
        })

    # Code validation
    if name == "Validate Code":
        mapping = {
            "Validate JSON": "JSON",
            "Validate Markdown": "Markdown",
            "Ensure LF line endings": "LF line endings",
        }
        for st in (j.get("steps") or []):
            stname = st.get("name")
            if stname in mapping:
                rows.append({
                    "group": "validation",
                    "name": mapping[stname],
                    "status": st.get("conclusion") or "skipped",
                    "duration": dur,
                    "url": j.get("html_url"),
                })

    # Tests matrix
    if name.startswith("Test "):
        pretty = _test_pretty_name(name)
        steps = j.get("steps") or []
        test_step = next((s for s in steps if s.get("name") == "Test"), None)
        status = (test_step.get("conclusion") if test_step else None) or j.get("conclusion") or "neutral"
        rows.append({
            "group": "tests",
            "name": pretty,
            "status": status,
            "duration": dur,
            "url": j.get("html_url"),
        })

    return rows

def collect_validation_and_tests() -> None:
    token = env("GITHUB_TOKEN")
    repo_full = env("GITHUB_REPOSITORY")  # "owner/repo"
    run_id = env("GITHUB_RUN_ID")

    if not (token and repo_full and run_id):
        print("[INFO] Missing GITHUB_TOKEN / GITHUB_REPOSITORY / GITHUB_RUN_ID; skipping GH API collect.")
        return

    owner, repo = repo_full.split("/", 1)
    r = gh_api(f"/repos/{owner}/{repo}/actions/runs/{run_id}/jobs",
               method="GET", token=token, params={"per_page": 100})
    jobs = r.get("jobs") or []

    rows: List[Dict[str, Any]] = []
    for j in jobs:
        rows.extend(_rows_for_job(j))

    pathlib.Path("partials").mkdir(parents=True, exist_ok=True)
    write_json_file("partials/validation.json", rows)

# ----------------------- 2) Compose Summary -----------------------

def _load_primary_items() -> List[Dict[str, Any]]:
    files = [p for p in glob.glob("partials/**/*.json", recursive=True)
             if not any(p.endswith(x) for x in ("validation.json", "source.json"))
             and not pathlib.Path(p).name.startswith("installer-")]
    items: List[Dict[str, Any]] = []
    for p in files:
        data = read_json_file(p)
        if isinstance(data, dict) and "family" in data:
            items.append(data)
    return items

def _load_installer_map() -> Dict[str, str]:
    inst_map: Dict[str, str] = {}
    for p in glob.glob("partials/installer-*.json"):
        obj = read_json_file(p) or {}
        plat, url = obj.get("platform"), obj.get("installer_url")
        if plat and url:
            inst_map[plat] = url
    return inst_map

def _durations_map(val_rows: List[Dict[str, Any]]) -> Dict[str, str]:
    return {r["name"]: r.get("duration", "-") for r in val_rows if r.get("group") == "builds"}

def _render_family_table(fam: str, rows: List[Dict[str, Any]],
                         inst_map: Dict[str, str], dur_map: Dict[str, str]) -> None:
    title, icon = family_title_and_icon(fam)
    append_summary(f'### <img src="{icon}" width="22"/> {title}\n\n')

    cols_arch = ["| Architecture |"]
    cols_stats = ["| Cache statistic |"]
    cols_time = ["| Build time |"]
    cols_down = ["| Download |"]

    for it in rows:
        plat = it.get("platform", "")
        arch = it.get("arch", "")
        hits = it.get("hits", "")
        total = it.get("total", "")
        rate = it.get("rate", "")
        hms = dur_map.get(plat, "-")

        main = it.get("artifact_url") or ""
        dbg  = it.get("debug_symbols_url") or ""
        aab  = it.get("aab_url") or ""
        inst = inst_map.get(plat, "")

        cols_arch.append(f" {arch} |")
        cols_stats.append(f" {rate} ({hits} / {total}) |")
        cols_time.append(f" {hms} |")

        dl_parts = []
        if inst: dl_parts.append(f"[Installer]({inst})")
        if dbg:  dl_parts.append(f"[Debug symbols]({dbg})")
        if main: dl_parts.append(f"[Archive]({main})")
        if aab:  dl_parts.append(f"[AAB]({aab})")
        dl = "<br/>".join(dl_parts) if dl_parts else "—"
        cols_down.append(f" {dl} |")

    count = len(rows)
    align = "|:--|" + ":--:|" * count

    append_summary("".join(cols_arch) + "\n")
    append_summary(align + "\n")
    append_summary("".join(cols_stats) + "\n")
    append_summary("".join(cols_time) + "\n")
    append_summary("".join(cols_down) + "\n\n")

def _render_validation_section(val_rows: List[Dict[str, Any]]) -> None:
    rows = sorted((r for r in val_rows if r.get("group") == "validation"),
                  key=lambda r: (VALIDATION_ORDER.get(r.get("name"), 999), r.get("name", "")))
    if not rows:
        return

    append_summary("### 🔍 Validation\n")
    append_summary("| Check | Status | Time | Logs |\n")
    append_summary(ALIGN_4_COLS)

    for r in rows:
        icon = status_icon(r.get("status", ""))
        dur = r.get("duration") or "-"
        url = r.get("url") or ""
        logs = f"[Logs]({url})" if url else "—"
        append_summary(f"| {r.get('name','')} | {icon} | {dur} | {logs} |\n")
    append_summary("\n")

def _render_tests_section(val_rows: List[Dict[str, Any]]) -> None:
    rows = sorted((r for r in val_rows if r.get("group") == "tests"),
                  key=lambda r: (TESTS_ORDER.get(r.get("name"), 999), r.get("name", "")))
    if not rows:
        return

    append_summary("### 🧪 Tests\n")
    append_summary("| Matrix | Status | Time | Logs |\n")
    append_summary(ALIGN_4_COLS)

    for r in rows:
        icon = status_icon(r.get("status", ""))
        dur = r.get("duration") or "-"
        url = r.get("url") or ""
        logs = f"[Logs]({url})" if url else "—"
        append_summary(f"| {r.get('name','')} | {icon} | {dur} | {logs} |\n")
    append_summary("\n")

def _render_build_matrix_section(val_rows: List[Dict[str, Any]]) -> None:
    rows = sorted((r for r in val_rows if r.get("group") == "builds"),
                  key=lambda r: r.get("name", ""))
    if not rows:
        return

    append_summary("### 🚦 Build matrix\n")
    append_summary("| Platform | Status | Time | Logs |\n")
    append_summary(ALIGN_4_COLS)

    for r in rows:
        icon = status_icon(r.get("status", ""))
        dur = r.get("duration") or "-"
        url = r.get("url") or ""
        logs = f"[Logs]({url})" if url else "—"
        append_summary(f"| {r.get('name','')} | {icon} | {dur} | {logs} |\n")
    append_summary("\n")

def compose_summary() -> None:
    # Source code section
    src_json = read_json_file("partials/source.json")
    if src_json and src_json.get("source_url"):
        append_summary("\n\n")
        append_summary(
            f'### <img src="{ICON_CPP}" width="20"/> Source code - [Download]({src_json["source_url"]})\n\n\n'
        )

    items = _load_primary_items()
    inst_map = _load_installer_map()
    val_rows: List[Dict[str, Any]] = read_json_file("partials/validation.json") or []
    dur_map = _durations_map(val_rows)

    # Family tables
    for fam in FAMILIES:
        fam_rows = [x for x in items if x.get("family") == fam]
        if fam_rows:
            _render_family_table(fam, fam_rows, inst_map, dur_map)

    # Validation, Tests, Build matrix
    _render_validation_section(val_rows)
    _render_tests_section(val_rows)
    _render_build_matrix_section(val_rows)

# ----------------------- 3) Delete partial artifacts -----------------------

def delete_partial_artifacts() -> None:
    token = env("GITHUB_TOKEN")
    repo_full = env("GITHUB_REPOSITORY")
    run_id = env("GITHUB_RUN_ID")
    if not (token and repo_full and run_id):
        print("[INFO] Missing env for deleting artifacts; skipping.")
        return

    owner, repo = repo_full.split("/", 1)
    r = gh_api(f"/repos/{owner}/{repo}/actions/runs/{run_id}/artifacts",
               token=token, params={"per_page": 100})
    arts = r.get("artifacts") or []
    for a in arts:
        name = a.get("name", "")
        if name.startswith("partial-json-"):
            aid = a.get("id")
            print(f"Deleting artifact {name} (id={aid})")
            gh_api(f"/repos/{owner}/{repo}/actions/artifacts/{aid}",
                   method="DELETE", token=token)

# ----------------------- Main -----------------------

def main() -> None:
    pathlib.Path("partials").mkdir(parents=True, exist_ok=True)

    try:
        collect_validation_and_tests()
    except Exception as e:
        print(f"[WARN] collect_validation_and_tests failed: {e}")

    try:
        compose_summary()
    except Exception as e:
        print(f"[WARN] compose_summary failed: {e}")

    try:
        delete_partial_artifacts()
    except Exception as e:
        print(f"[WARN] delete_partial_artifacts failed: {e}")

if __name__ == "__main__":
    main()
