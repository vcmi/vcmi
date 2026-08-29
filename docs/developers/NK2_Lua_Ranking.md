# Nullkiller Lua ranking policies

Nullkiller can optionally replace its compiled candidate scores with scores from a Lua policy. The default remains the compiled `PriorityEvaluator`; no Lua state is created and no policy data is collected unless a policy is explicitly selected.

The bundled `config/ai/nk2ai/policies/reference.lua` is a readable translation of the complete compiled ranking algorithm. It covers all priority tiers and is intended to produce the same acceptance decisions and scores as the pure C++ `evaluatePriority` scorer. Keep it free of experimental tuning: copy it when starting a new policy.

## Selecting a policy

Set `VCMI_NK2_POLICY` to a JSON policy profile before starting VCMI:

```sh
VCMI_NK2_POLICY=/path/to/reference.json vcmiclient
```

Use a color-suffixed variable such as `VCMI_NK2_POLICY_RED` or `VCMI_NK2_POLICY_BLUE` to select a different policy for one player. Color names are uppercase: `RED`, `BLUE`, `TAN`, `GREEN`, `ORANGE`, `PURPLE`, `TEAL`, and `PINK`. The per-player value takes precedence over `VCMI_NK2_POLICY`. An unset value or `builtin` uses the compiled evaluator.

A profile has this format:

```json
{
	"schemaVersion": 1,
	"name": "my-policy",
	"type": "lua",
	"script": "my-policy.lua",
	"shadow": false,
	"parameters": {}
}
```

Relative script paths are resolved from the profile directory. Parameters are copied to `request.parameters`.

The bundled profiles are:

- `reference.json`: apply the reference Lua scores.
- `reference-shadow.json`: run the same policy and compare it with compiled scores, but keep using the compiled scores.

Shadow mode logs score or acceptance mismatches. It is useful when changing the compiled evaluator, the request schema, or the reference policy.

## Lua contract

A script returns a table with a `rank` function:

```lua
local Policy = {}

function Policy.rank(request)
	local scores = {}
	for _, candidate in ipairs(request.candidates) do
		table.insert(scores, {
			id = candidate.id,
			score = candidate.baselinePriority,
		})
	end
	return { scores = scores }
end

return Policy
```

The result must contain every candidate ID exactly once. IDs must be non-negative integers and scores must be finite numbers. A score at or below zero rejects a candidate in the same way as the compiled evaluator. Invalid output or a Lua error leaves the compiled ranking unchanged.

The request uses schema version 1 and contains:

- `player`, `playerId`, and `day`.
- `priorityTier` and `priorityTierId`. Prefer the named tier: `buildings`, `instakill`, `instaDefend`, `kill`, `escape`, `exploreAndGather`, or `defend`.
- `parameters`, copied from the selected profile.
- `state`, the player's value-only strategic state.
- `candidates`, including their compiled scores and complete ranking inputs.

`state` contains:

- Named resource objects: `resources`, `dailyIncome`, and `lockedResources`. Their keys are `wood`, `mercury`, `ore`, `sulfur`, `crystal`, `gems`, and `gold`.
- Resource summaries: `resourceMarketValue`, `lockedResourceMarketValue`, `goldPressureOverMax`, and `hasTownWithoutMarketplace`.
- Risk and calendar data: `maximumArmyLossTarget`, `daysWithoutCastle`, `dayOfWeek`, and `daysInWeek`.
- Progress counts: heroes, towns, owned and known objects, revealed tiles, buildings, fort levels, experience, levels, and army strengths.
- Per-hero values: ID, level, experience, army and total strength, movement, and mana.
- Per-town values: ID, building, fort, hall, and mage-guild levels, army strength, named daily income, and marketplace availability.

Each candidate contains:

- `id`, `goalType`, `goalTypeId`, and a diagnostic `description`.
- `initialPriority`, `baselinePriority`, and `baselineAccepted`.
- Optional `heroId`, `townId`, and `objectId`, plus the target coordinates.
- `rankingInput`, the exact value-only input shared by the C++ and Lua scorers. It contains the named tier, scorer-specific state, evaluation values, and escape threat values. Resource maps use the names `wood`, `mercury`, `ore`, `sulfur`, `crystal`, `gems`, and `gold`.
- `evaluation`, a convenience copy of the evaluation part of `rankingInput`, extended with movement by role, mana cost, defense value, and sailing status for experimental policies.

The request contains copies of these values, so policies can use NK2's calculated features without repeating expensive analysis. Policies also run in VCMI's normal Lua context and receive its standard globals:

- `GAME`, the read-only query interface for the current game.
- `LIBRARY`, the static game-content catalogue.
- `ENUM`, the engine's named enumerations.
- `require`, for loading Lua modules from the VCMI virtual filesystem.
- `print`, redirected to the VCMI log.

This lets experimental policies combine the stable ranking request with game details and reusable Lua modules. It does not grant a server callback or direct game-state mutation.

## Lua environment and reproducibility

The policy uses the same Lua host, bindings, standard libraries, and safety-related global cleanup as other VCMI scripts. The `io`, `os`, `package`, `debug`, dynamic-loading, garbage-collection, and random-number APIs are unavailable. VCMI does not impose policy-specific memory or instruction limits.

The bundled reference policy is deterministic, stateless, and reads only the request. Therefore captured ranking requests are complete differential-test inputs. Experimental policies may query `GAME` or retain Lua state, but then request fixtures alone cannot reproduce their decisions; validate them from complete saves with the same AI memory and policy version. Stateless policies remain preferable because they also reproduce cleanly across save and load.

Lua normally calculates with doubles while NK2 stores scores as C++ `float`. The host-provided `float32(value)` helper rounds intermediate results to a C++ float. The reference policy uses it at the same points as the compiled algorithm to preserve parity.

## Maintaining the reference

The differential tests store inputs, not expected scores. For every fixture they evaluate the committed input with the current C++ scorer, pass that same input to Lua, and compare the outputs. This makes the compiled implementation the live oracle and prevents an old golden value from allowing the implementations to drift together unnoticed.

When `evaluatePriority` changes:

1. Make the equivalent change in `reference.lua` without adding tunable behavior.
2. Run the `Nullkiller2_Engine_DecisionPolicy` tests. They exercise all seven tiers and compare Lua directly with the current compiled scorer over captured inputs.
3. Run the reference shadow profile over representative maps and days. Accept no score, acceptance, or selected-candidate mismatches.

Experimental policies should live in separate files. This keeps the reference a stable baseline for reviewing and measuring later policy changes.
