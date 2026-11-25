# RMG Agent Notes (`lib/rmg`)

This file describes practical rules for changes in [`lib/rmg`](./).
Keep it short, and avoid duplicating long architecture writeups that already exist elsewhere.

## Scope

- This file applies to [`lib/rmg`](./).
- Modifier-specific details belong in [`./modificators/AGENTS.md`](./modificators/AGENTS.md).

## Canonical References

- High-level RMG algorithm description: [`../../docs/developers/RMG_Description.md`](../../docs/developers/RMG_Description.md)
- Template schema: [`../../config/schemas/template.json`](../../config/schemas/template.json)
- Runtime tuning values: [`../../config/randomMap.json`](../../config/randomMap.json)

When this file and source code disagree, trust source code first.

## Main Entry Points

- [`CMapGenerator.cpp`](./CMapGenerator.cpp) / [`CMapGenerator.h`](./CMapGenerator.h): generation orchestration (`generate`, zone generation, zone fill, finalization).
- [`RmgMap.cpp`](./RmgMap.cpp) / [`RmgMap.h`](./RmgMap.h): runtime map state and modifier registration.
- [`Zone.cpp`](./Zone.cpp) / [`Zone.h`](./Zone.h): per-zone runtime data and area state.
- [`CRmgTemplate.cpp`](./CRmgTemplate.cpp) / [`CRmgTemplate.h`](./CRmgTemplate.h): template loading/inheritance logic.
- [`CMapGenOptions.cpp`](./CMapGenOptions.cpp) / [`CMapGenOptions.h`](./CMapGenOptions.h): option finalization and template selection.
- [`CZonePlacer.cpp`](./CZonePlacer.cpp) / [`CZonePlacer.h`](./CZonePlacer.h): zone placement and tile assignment.
- [`CRoadRandomizer.cpp`](./CRoadRandomizer.cpp) / [`CRoadRandomizer.h`](./CRoadRandomizer.h): resolves `road=random` links before modifier execution.

## Working Rules

- Keep pipeline ordering intact unless you verify downstream assumptions in [`CMapGenerator.cpp`](./CMapGenerator.cpp) and [`RmgMap.cpp`](./RmgMap.cpp).
- Prefer small, localized edits. RMG has many implicit dependencies between steps.

## Invariants To Preserve

- Zone ownership mapping (`map.getZoneID`) stays consistent with terrain/area transfers.
- Zone area sets remain coherent (`area`, `areaPossible`, `freePaths`, `areaUsed`) after each generation step.
- Connection intent from template edges is still realized (direct passage, water routing, subterranean gate, or portal route, depending on settings).
- Generated objects stay within map bounds and keep path connectivity assumptions used by later stages.

## Change Checklists

### If you add/change template fields

1. Update zone/template structures in [`CRmgTemplate.h`](./CRmgTemplate.h) or related option structs.
2. Update JSON serialization/parsing in [`CRmgTemplate.cpp`](./CRmgTemplate.cpp) and storage code.
3. Update inheritance/finalization logic (`afterLoad`, option finalization paths).
4. Update schema in [`../../config/schemas/template.json`](../../config/schemas/template.json).

### If you change placement or connectivity behavior

1. Update implementation in [`CZonePlacer.cpp`](./CZonePlacer.cpp), [`CRoadRandomizer.cpp`](./CRoadRandomizer.cpp), or relevant map/zone code.
2. Verify interactions with connection building and area ownership transitions.
3. Update [`../../docs/developers/RMG_Description.md`](../../docs/developers/RMG_Description.md) only when behavior-level docs become stale.

### If you work on modifiers

1. Read [`./modificators/AGENTS.md`](./modificators/AGENTS.md).
2. Register/adjust modifier wiring in [`RmgMap.cpp`](./RmgMap.cpp).
3. Re-check dependency ordering and lock usage.

## Checking Generated Maps

Unit tests cover single functions. To see what a change does to whole maps (object counts, rewards, bans, timing), generate real maps with the map editor's headless mode. Do not write a temporary gtest that runs `CMapGenerator`.

```sh
vcmieditor --generate /abs/path/out.vmap --template 'Jebus Cross' --seed 12345 --single-thread
```

- Build the map editor (`vcmieditor` target, needs Qt), then run the binary from the build's `bin/` directory: `build/bin/vcmieditor` on Linux and macOS, `build\bin\<Config>\VCMI_mapeditor.exe` on Windows. In an installed VCMI it is `vcmieditor` (`VCMI_mapeditor.exe` on Windows).
- Pass an absolute output path when a wrapper script starts the editor from another directory; relative paths resolve against the directory the editor was started in.
- It needs no display. It loads the same data and mods as the editor, so templates from mods are available.
- Options: `--template`, `--width`/`--height` (both or neither), `--levels`, `--players`, `--computer-players`, `--water random|none|normal|islands` (default `none`; a template that needs water picks its own), `--monsters random|weak|normal|strong`, `--seed`, `--single-thread`, `--dump <stage[,stage...]>`, `--log <domain>=<level>`. `vcmieditor --help` lists them.
- `--template` matches the exact id (`<mod scope>:<name>`) first, then the exact name, then part of the id or name. When several templates match, the error lists their ids.
- Exit codes: 0 when the map is saved, 1 when generation or saving fails, 2 for bad arguments, no matching template, or an option the template does not allow. The command never silently changes the size, player count or water you asked for.
- The last line of stdout reports the saved path, template id, seed, size and `generate_ms` (time spent in `CMapGenerator::generate()`).
- Generation is multithreaded by default, so the same seed can give different maps. When comparing two builds, always pass `--single-thread` and the same `--seed`; then the same seed gives the same map.
- A `.vmap` is a zip of JSON files: `header.json`, `objects.json` and one terrain file per level. `creationDateTime` in `header.json` differs on every run; ignore it when comparing maps.
- Each generation rewrites `VCMI_RMG_log.txt` in the user logs directory. `--dump <stage[,stage...]>` writes whole-map ASCII snapshots under `rmg-dumps/` there after each named stage (`zones` or a modificator class name). `--log <domain>=<level>` raises a log level for that run; `rmg.zonePlacement=trace` includes per-iteration zone placement. Neither option writes `settings.json`.
