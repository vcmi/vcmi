# Damage Calculator Script

Declared with `"implements" : "damageCalculator"` in the [scripts](Script_Types.md) section of a mod. This script calculates the damage of an attack - the damage of the creatures, everything that raises or lowers it, and an estimate of how many creatures die.

Unlike the other script types there is exactly **one** damage calculator in a game. It is not attached to a unit and is not granted by a bonus: the engine calls it for every attack, both when the blow is dealt and when an AI only estimates it. VCMI ships `core:damageCalculator`, and a mod changes the rules by [stacking a patch](#changing-a-rule) over it instead of declaring one of its own.

```json
"damageCalculator" : {
    "implements" : "damageCalculator",
    "script" : "damage/damageCalculator",
    "patches" : [ "damage/magicElemental", "damage/psychicElemental", ... ],
    "schema" : { "properties" : {}, "additionalProperties" : false }
}
```

## How damage is worked out

Every attack goes through the same three steps.

**1. Base damage.** Damage of the creatures themselves, min and max, multiplied by how many of them are alive. Bless and curse collapse that range onto one of its ends.

**2. Factors.** Everything that scales base damage is a *factor* - a signed share of the base damage. **Positive raises it, negative lowers it.** How a factor applies is decided by its sign alone, not by its source:

- factors that raise the damage **add up**: attack over defence (+5% per point), offence (+30%), luck (+100%) give `1 + 0.05×points + 0.3 + 1.0`
- factors that lower it **multiply**, each taking its share of what is left: armourer (-15%) and a shooting penalty (-50%) give `0.85 × 0.5`

The two totals are multiplied together. This is why a single -50% never halves the damage twice, and why giving a "boost" a negative value turns it into a mitigation instead of cancelling out other boosts.

**3. Casualties.** How many creatures the resulting damage kills, given the health left on the first one. This is only used for damage preview in UI, and for AI estimation - engine instead rolls damage within specified range.

## Adding a factor

Write a patch, list it in `patches`, write the factor as a method of it, and hand its name to `addDamageFactor`:

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

Two lines register it: `declareBonus` for every bonus type the factor reads - see [declaring what you look at](#declaring-what-you-look-at) - and `addDamageFactor` for the factor itself. The order in which factors are added does not matter; a factor is applied according to its sign, so **return a negative number to lower the damage** and a positive one to raise it.

`addDamageFactor` takes the *name* of the method, not the method itself, so that a patch stacked later can override it.

## Changing a rule

Every step is a method and can be overridden, the factors of the base script among them - `getBaseDamageSingle`, `getBaseDamageBlessCurse`, `getAttack`, `getDefense`, `getDamageCap`, `getCasualties`, `getJoustingFactor`, `getArmorerFactor`, ...

```lua
--- Make Jousting twice stronger, from any source.
function Script:getJoustingFactor(info)
    return Base.getJoustingFactor(self, info) * 2
end
```

Call up the chain with `Base.method(self, ...)` - a dot and an explicit `self`. Writing `self:method(...)` dispatches back into your own patch and loops forever.

Some steps exist only to be patched. `getAttackIgnored` and `getDamageCap` return "nothing" in the base script, because no Heroes 3 rule lowers the attack of whoever strikes a unit or caps the damage of a blow; the rules that do are in `damage/enemyAttackReduction` and `damage/damageReceivedCap`. Read those two for the shortest example of a patch, and `damage/vulnerableFromBack` for one that adds a factor.

Each patch implements a single rule, so that a mod can drop or replace one of them without touching the rest. This is not required, but is recommended for mod patches as well.

## What the script is given

`Script:calculate(battle, info)` receives the battle and one table describing the attack:

- `attacker`, `defender` - the two units. See [Unit](../Lua_Reference/Unit.md)
- `attackerHex`, `defenderHex` - where the blow happens. Note that this position may differ from position reported by units - if this is estimation, and units are still at their old positions.
- `shooting`, `luckyStrike`, `unluckyStrike`, `deathBlow`, `doubleDamage` - what kind of blow this is. Random roll-based abilities are only set when actual calculation is performed by server
- `chargeDistance` - hexes crossed to reach the target, used by jousting
- `attackerBonuses`, `defenderBonuses` - which of the [declared bonus types](#declaring-what-you-look-at) each unit carries. Read them through `self:hasBonusOfType(info.attackerBonuses, "JOUSTING")`
- `attackFactorPerPoint`, `attackFactorCap`, `defenseFactorPerPoint`, `defenseFactorCap` - the tuning constants from `gameConfig.json`, so the script needs no access to settings

It returns a table of three ranges:

```lua
return {
    damage = { min = ..., max = ... },
    kills = { min = ..., max = ... },
    damageBeforeDefense = { min = ..., max = ... }
}
```

`damageBeforeDefense` is the damage the blow would deal if the target had no defences at all. Abilities that reflect a strike, such as fire shield, are calculated from it - see `damageBeforeDefense` in [combat event scripts](Combat_Event_Scripts.md).

## Declaring what you look at

Reading a bonus means a call into the engine, across the language boundary. To avoid doing this twenty times per attack, the script declares which bonus types it reads, and the engine reports which of them each unit actually carries:

```lua
Script:declareBonus("VULNERABLE_FROM_BACK")
```

A patch **must declare every bonus type its factor reads**, or the check will not find it. Reading a type that was never declared raises an error naming that type, instead of silently reporting it as absent and changing the damage.

## Writing a factor that does not slow the game down

This script runs on every attack the game resolves **and on every attack an AI considers** - some two hundred thousand times per AI turn in a large battle. A slow factor shows up as a longer AI turn rather than as a dropped frame, so it is worth knowing which lines are cheap and which are not.

**Reading `info` is free. Calling into the engine is not.** Any `:` call on a unit, a bonus or the battle crosses into the engine and back. Reading a field of `info`, or of the two bonus tables, is a plain table lookup.

**Check the bonus table before any engine call.** This is the single most useful habit: most units carry none of the bonuses a given factor reads, and the table reports that without leaving the script.

```lua
-- good: the query only happens for a unit that actually has the bonus
if not self:hasBonusOfType(info.attackerBonuses, "JOUSTING") then return 0 end

return info.chargeDistance * info.attacker:getBonusesValue({type = "JOUSTING"}) / 100
```

`hasBonusOfType` reads the same table you could read yourself - `info.attackerBonuses.JOUSTING` does the same job - but it also raises an error when the type was never declared, instead of silently reporting it as absent.

Four helpers combine the check and the query, so a factor rarely needs to write both:

| function | description |
| -------- | ----------- |
| `self:hasBonusOfType(present, type)` | whether the unit carries it at all |
| `self:getBonusValueOfType(unit, present, type)` | total value of the bonuses of that type |
| `self:getBonusValueOfSubtype(unit, present, type, subtype)` | the same, narrowed to one subtype |
| `self:getBonusValueOfTypeAndRange(unit, present, type, shooting)` | the same, counting only bonuses that apply to this kind of blow |

Each returns 0 without calling the engine when the snapshot reports the type as absent, which is the usual case. `present` is `info.attackerBonuses` or `info.defenderBonuses`, for the unit being queried.

**Put the cheapest test first.** Conditions are evaluated left to right, so order them by what they cost:

```lua
-- good: a table read rules out almost every unit before anything is asked
if not self:hasBonusOfType(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end
if info.attacker:getCreature():getJsonKey() ~= "core:psychicElemental" then return 0 end
```

**Query a value, not a list.** `getBonusesValue` returns the total value of the matching bonuses, computed by the engine in a single call. Fetching the list and adding up `getVal()` costs one call for the list and one more per bonus in it, and gives a wrong result when bonuses do not simply add up (percentages, independent floors and ceilings).

```lua
-- good
local armour = info.defender:getBonusesValue({type = "GENERAL_DAMAGE_REDUCTION"})

-- bad: more crossings, and wrong for anything that is not plain addition
local list = info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION"})
local armour = 0
for i = 1, list:size() do armour = armour + list:getBonus(i):getVal() end
```

**Query whether, not which**, when that is all you need. `hasBonuses` returns true or false without building the list for the script:

```lua
-- good
if info.defender:hasBonuses({type = "MIND_IMMUNITY"}) then ... end

-- bad: the whole list is handed over just to be counted
if info.defender:getBonuses({type = "MIND_IMMUNITY"}):size() > 0 then ... end
```

**Put as much as possible into the filter.** Type, subtype, source and kind of blow are all matched by the engine, and a query the engine can describe is also one it can cache. Only conditions the filter cannot express - "from anything except a spell" - belong in a `filter` afterwards:

```lua
-- good: the engine finds them
info.defender:getBonusesValue({
    type = "GENERAL_DAMAGE_REDUCTION",
    subtype = "damageTypeAll",
    sourceType = ENUM.BonusSource.spellEffect
})

-- only when the filter cannot say it
info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION"}):filter(function(bonus)
    return bonus:getSource() ~= ENUM.BonusSource.spellEffect
end):totalValue()
```

**`shooting` excludes bonuses that do not apply to this blow.** A bonus limited to melee is absent from a shot and the other way round, and a bonus limited to neither always counts. Pass the flag of the attack through instead of reading `getEffectRange` yourself:

```lua
-- good
info.defender:getBonusesValue({type = "ENEMY_ATTACK_REDUCTION", shooting = info.shooting})
```

The filter takes the kind of blow, not an effect range, because "applies in melee" covers two effect ranges at once. Querying them one at a time would add the two results up instead of combining them the way the engine does.

**Do not build tables you do not need.** A factor that returns 0 for most attacks should return it before creating anything.

Everything else - arithmetic, comparisons, local variables - is cheap enough to ignore. Write the calculation plainly; it is the engine calls that add up.
