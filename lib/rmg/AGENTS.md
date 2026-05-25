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
