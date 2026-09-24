# Damage Calculator Script

`"implements" : "damageCalculator"` in a mod's [scripts](Script_Types.md) section declares a damage calculator script. It calculates attack damage ranges and casualty estimates.

Each game uses one damage calculator. It is not attached to a unit or granted by a bonus. The engine calls it for every resolved or estimated attack. VCMI provides `core:damageCalculator`; mods change its rules with [patches](#changing-a-rule).

```json
"damageCalculator" : {
    "implements" : "damageCalculator",
    "script" : "damage/damageCalculator",
    "patches" : [ "damage/magicElemental", "damage/psychicElemental", ... ],
    "schema" : { "properties" : {}, "additionalProperties" : false }
}
```

## Damage calculation

Every attack goes through the same three steps.

**1. Base damage.** Minimum and maximum creature damage multiplied by the alive creature count. Bless and curse select one end of the range.

**2. Factors.** Each factor is a signed share of base damage. Positive values increase damage and negative values reduce it. The sign determines how the factor is applied:

- factors that raise the damage **add up**: attack over defence (+5% per point), offence (+30%), luck (+100%) give `1 + 0.05×points + 0.3 + 1.0`
- factors that lower it **multiply**, each taking its share of what is left: armourer (-15%) and a shooting penalty (-50%) give `0.85 × 0.5`

The two totals are multiplied together. Each negative factor is applied once. A negative boost becomes a mitigation factor and does not cancel positive factors.

**3. Casualties.** Killed creature count based on the resulting damage and remaining health of the first creature. UI previews and AI evaluation use this estimate; resolved attacks roll within the damage range.

## Adding a factor

Define the factor in a patch listed in `patches`, then register its method name with `addDamageFactor`:

```lua
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Some creatures take more damage when attacked from behind
function Script:getFromBackFactor(info)
	local value = self:getBonusValueOfType(info.defender, info.defenderBonuses, "VULNERABLE_FROM_BACK")

	if value == 0 then return 0 end
	if not info.battle:isToReverse(info.attacker, info.defender, info.attackerHex, info.defenderHex) then return 0 end

	return value / 100
end

Script:declareBonus("VULNERABLE_FROM_BACK")
Script:addDamageFactor("getFromBackFactor")

return Script
```

Call `declareBonus` for every bonus type read by the factor; see [Declaring bonus dependencies](#declaring-bonus-dependencies). Call `addDamageFactor` with the factor method name. Factor registration order does not affect calculation. Return a negative value to reduce damage and a positive value to increase it.

`addDamageFactor` takes the *name* of the method, not the method itself, so that a patch stacked later can override it.

## Changing a rule

Calculation stages and base factors are overridable methods, including `getBaseDamageSingle`, `getBaseDamageBlessCurse`, `getAttack`, `getDefense`, `getDamageCap`, `getCasualties`, `getJoustingFactor` and `getArmorerFactor`.

```lua
--- Make Jousting twice stronger, from any source.
function Script:getJoustingFactor(info)
    return Base.getJoustingFactor(self, info) * 2
end
```

Call the base implementation with `Base.method(self, ...)`. `self:method(...)` dispatches to the override and causes recursion.

Some steps exist only as patch points. `getAttackIgnored` and `getDamageCap` have neutral base implementations because Heroes 3 has no corresponding rules. `damage/enemyAttackReduction` and `damage/damageReceivedCap` are minimal patch examples; `damage/vulnerableFromBack` adds a factor.

Each patch implements a single rule, so that a mod can drop or replace one of them without touching the rest. This is not required, but is recommended for mod patches as well.

## Input data

`Script:calculate(battle, info)` receives the battle and one table describing the attack:

- `attacker`, `defender` - the two units. See [Unit](../Lua_Reference/Unit.md)
- `attackerHex`, `defenderHex` - positions used for this calculation. Estimated attacks may use positions different from the current unit positions
- `shooting`, `luckyStrike`, `unluckyStrike`, `deathBlow`, `doubleDamage` - attack flags. Random flags are set only for server-side resolved attacks
- `chargeDistance` - hexes crossed to reach the target, used by jousting
- `attackerBonuses`, `defenderBonuses` - presence tables for [declared bonus types](#declaring-bonus-dependencies). Read them through `self:hasBonusOfType(info.attackerBonuses, "JOUSTING")`
- `attackFactorPerPoint`, `attackFactorCap`, `defenseFactorPerPoint`, `defenseFactorCap` - the tuning constants from `gameConfig.json`, so the script needs no access to settings

It returns a table of three ranges:

```lua
return {
    damage = { min = ..., max = ... },
    kills = { min = ..., max = ... },
    damageBeforeDefense = { min = ..., max = ... }
}
```

`damageBeforeDefense` is the attack damage with target defences ignored. Reflected-damage abilities such as fire shield use it; see `damageBeforeDefense` in [combat event scripts](Combat_Event_Scripts.md).

## Declaring bonus dependencies

Bonus queries cross the Lua/C++ boundary. The script declares its bonus dependencies so the engine can provide a presence table for each unit:

```lua
Script:declareBonus("VULNERABLE_FROM_BACK")
```

A patch **must declare every bonus type it reads**. Querying an undeclared type raises an error.

## Writing a factor that does not slow the game down

This script runs on every resolved attack and every attack evaluated by AI, potentially hundreds of thousands of times per AI turn. Slow factors increase AI turn duration but do not affect rendering.

**Prefer Lua table reads.** Reading `info` or a bonus-presence table is a Lua table lookup. Calls on units, bonuses or the battle cross the Lua/C++ boundary.

**Check the bonus table before an engine call.** Most units do not have the bonus required by a specific factor, so the presence table avoids most engine calls.

```lua
-- Query only when the unit has the bonus.
if not self:hasBonusOfType(info.attackerBonuses, "JOUSTING") then return 0 end

return info.chargeDistance * info.attacker:getBonusesValue({type = "JOUSTING"}) / 100
```

`hasBonusOfType` reads the same table as `info.attackerBonuses.JOUSTING` and also validates that the bonus type was declared.

Four helpers combine the check and the query, so a factor rarely needs to write both:

| function | description |
| -------- | ----------- |
| `self:hasBonusOfType(present, type)` | whether the unit carries it at all |
| `self:getBonusValueOfType(unit, present, type)` | total value of the bonuses of that type |
| `self:getBonusValueOfSubtype(unit, present, type, subtype)` | the same, narrowed to one subtype |
| `self:getBonusValueOfTypeAndRange(unit, present, type, shooting)` | the same, counting only bonuses that apply to this attack type |

Each returns 0 without calling the engine when the snapshot reports the type as absent. `present` is `info.attackerBonuses` or `info.defenderBonuses` for the queried unit.

**Put the cheapest test first.** Conditions are evaluated from left to right:

```lua
-- A table read rejects most units before an engine call.
if not self:hasBonusOfType(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end
if info.attacker:getCreature():getJsonKey() ~= "core:psychicElemental" then return 0 end
```

**Query a value, not a list.** `getBonusesValue` returns the total value of the matching bonuses, computed by the engine in a single call. Fetching the list and adding up `getVal()` costs one call for the list and one more per bonus in it, and gives a wrong result when bonuses do not simply add up (percentages, independent floors and ceilings).

```lua
-- One engine call.
local armour = info.defender:getBonusesValue({type = "GENERAL_DAMAGE_REDUCTION"})

-- Multiple engine calls and incorrect handling of non-additive bonuses.
local list = info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION"})
local armour = 0
for i = 1, list:size() do armour = armour + list:getBonus(i):getVal() end
```

**Use `hasBonuses` for boolean queries.** It returns true or false without building a bonus list:

```lua
-- Boolean query.
if info.defender:hasBonuses({type = "MIND_IMMUNITY"}) then ... end

-- Builds a list only to count it.
if info.defender:getBonuses({type = "MIND_IMMUNITY"}):size() > 0 then ... end
```

**Put as much as possible into the filter.** The engine matches type, subtype, source and attack type. Declarative queries can also be cached. Use a subsequent `filter` only for conditions the query cannot express, such as "from anything except a spell":

```lua
-- Fully declarative query.
info.defender:getBonusesValue({
    type = "GENERAL_DAMAGE_REDUCTION",
    subtype = "damageTypeAll",
    sourceType = ENUM.BonusSource.spellEffect
})

-- Lua filter for a condition not supported by the query.
info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION"}):filter(function(bonus)
    return bonus:getSource() ~= ENUM.BonusSource.spellEffect
end):totalValue()
```

**`shooting` excludes bonuses that do not apply to the attack.** A melee-only bonus is excluded from shots, a ranged-only bonus is excluded from melee attacks, and an unrestricted bonus always applies. Pass the attack flag through instead of reading `getEffectRange` directly:

```lua
-- Attack-type filter.
info.defender:getBonusesValue({type = "ENEMY_ATTACK_REDUCTION", shooting = info.shooting})
```

The filter takes an attack type because "applies in melee" covers two effect ranges. Separate effect-range queries would add their results instead of using the engine's combination rules.

**Do not build tables you do not need.** A factor that returns 0 for most attacks should return it before creating anything.

Arithmetic, comparisons and local variables have negligible cost here. Engine calls dominate factor overhead.
