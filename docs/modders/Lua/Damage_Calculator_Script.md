# Damage Calculator Script

Declared with `"implements" : "damageCalculator"` in the [scripts](Script_Types.md) section of a mod. This script decides what an attack is worth - the damage of the creatures, everything that raises or lowers it, and estimation on how many creatures die.

Unlike the other script types there is exactly **one** damage calculator in a game. It is not attached to a unit and nothing grants it: the engine asks it about every attack, whether the blow is being dealt or an AI is only weighing it. VCMI ships `core:damageCalculator`, and a mod changes the rules by [stacking a patch](#changing-a-rule) over it rather than by declaring one of its own.

```json
"damageCalculator" : {
    "implements" : "damageCalculator",
    "script" : "damage/damageCalculator",
    "patches" : [ "damage/siegeWeapon", "damage/magicElemental", ... ],
    "schema" : { "properties" : {}, "additionalProperties" : false }
}
```

## How damage is worked out

Every attack goes through the same three steps.

**1. Base damage.** What the creatures themselves deal, min and max, multiplied by how many of them are alive. Bless and curse collapse that range onto one of its ends, a ballista multiplies it by the attack of its hero.

**2. Factors.** Everything that scales base damage is a *factor* - a signed share of the base damage. **Positive raises it, negative lowers it.** What decides how a factor applies is its sign alone, not where it came from:

- factors that raise the damage **add up**: attack over defence (+5% per point), offence (+30%), luck (+100%) give `1 + 0.05×points + 0.3 + 1.0`
- factors that lower it **multiply**, each taking its share of what is left: armourer (-15%) and a shooting penalty (-50%) give `0.85 × 0.5`

The two totals are multiplied together. This is why a single -50% never quite halves the damage twice, and why giving a "boost" a negative value turns it into a mitigation rather than cancelling out other boosts.

**3. Casualties.** How many creatures the resulting damage kills, given the health left on the first one. This is only used for damage preview in UI, and for AI estimation - engine instead rolls damage within specified range.

## Adding a factor

Write a patch, list it in `patches`, write the factor as a method of it, and hand its name to `addDamageFactor`:

```lua
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Some creatures take more from a blow they never saw coming
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

Two lines register it: `declareBonus` for every bonus type the factor reads - see [declaring what you look at](#declaring-what-you-look-at) - and `addDamageFactor` for the factor itself. The order factors are added in does not matter; what a factor is worth is decided by its sign, so **return a negative number to lower the damage** and a positive one to raise it.

`addDamageFactor` is given the *name* of the method rather than the method itself, so that a patch stacked later can override it and be the one that runs.

## Changing a rule

Every step is a method and can be overridden, the factors of the base script among them - `getBaseDamageSingle`, `getBaseDamageBlessCurse`, `getAttack`, `getDefense`, `getDamageCap`, `getCasualties`, `getJoustingFactor`, `getArmorerFactor`, ...

```lua
--- Make Jousting twice stronger, from any source.
function Script:getJoustingFactor(info)
    return Base.getJoustingFactor(self, info) * 2
end
```

Call up the chain with `Base.method(self, ...)` - a dot and an explicit `self`. Writing `self:method(...)` dispatches back into your own patch and loops forever.

Some steps exist only to be patched. `getAttackIgnored` and `getDamageCap` answer "nothing" in the base script, because nothing in Heroes 3 lowers the attack of whoever strikes it or caps the damage a blow may deal - the rules that do live in `damage/enemyAttackReduction` and `damage/damageReceivedCap`. Read those two for the shortest example of a patch, and `damage/vulnerableFromBack` for one that adds a factor.

Each patch keeps to one rule. That is what lets a mod drop or replace a single one of them without touching anything else, and while it is not required, it is worth following in mod patches too.

## What the script is given

`Script:calculate(battle, info)` receives the battle and one table describing the attack:

- `attacker`, `defender` - the two units. See [Unit](../Lua_Reference/Unit.md)
- `attackerHex`, `defenderHex` - where the blow happens. Note that this position may differ from position reported by units - if this is estimation, and units are still at their old positions.
- `shooting`, `luckyStrike`, `unluckyStrike`, `deathBlow`, `doubleDamage` - what kind of blow this is. Random roll-based abilities are only set when actual calculation is performed by server
- `chargeDistance` - hexes crossed to reach the target, which is what jousting scales with
- `attackerBonuses`, `defenderBonuses` - which of the [declared bonus types](#declaring-what-you-look-at) each unit carries. Read them through `self:hasBonusOfType(info.attackerBonuses, "JOUSTING")`
- `attackFactorPerPoint`, `attackFactorCap`, `defenseFactorPerPoint`, `defenseFactorCap` - the tuning constants from `gameConfig.json`, so the script needs no access to settings

It answers with a table of three ranges:

```lua
return {
    damage = { min = ..., max = ... },
    kills = { min = ..., max = ... },
    damageBeforeDefense = { min = ..., max = ... }
}
```

`damageBeforeDefense` is what the blow would have been worth had the target no defences at all. Abilities that reflect a strike, such as fire shield, work from it - see `damageBeforeDefense` in [combat event scripts](Combat_Event_Scripts.md).

## Declaring what you look at

Reading a bonus means asking the engine, and the engine is on the other side of the language boundary. To keep that from happening twenty times per attack, the script declares which bonus types it looks at, and the engine reports which of them each unit actually carries:

```lua
Script:declareBonus("VULNERABLE_FROM_BACK")
```

A patch **must declare whatever its factor looks at**, or the check will not find it. Asking about a type that was never declared raises an error naming it, rather than quietly answering "not there" and costing damage.

## Writing a factor that does not slow the game down

This script runs on every attack the game resolves **and on every attack an AI considers** - some two hundred thousand times per AI turn in a large battle. A factor that is careless about it is felt as the AI thinking longer, not as a dropped frame, so it is worth knowing which lines are cheap and which are not.

**Reading `info` is free. Calling into the engine is not.** Anything reached through a `:` on a unit, a bonus or the battle crosses into the engine and back. Reading a field of `info`, or of the two bonus tables, is a plain table lookup.

**Check the bonus table before you ask anything.** This is the single most useful habit: most units carry none of what a given factor looks for, and the table answers that without leaving the script.

```lua
-- good: the query only happens for a unit that actually has the bonus
if not self:hasBonusOfType(info.attackerBonuses, "JOUSTING") then return 0 end

return info.chargeDistance * info.attacker:getBonusesValue({type = "JOUSTING"}) / 100
```

`hasBonusOfType` reads the same table you could read yourself - `info.attackerBonuses.JOUSTING` does the same job - but it also complains when the type was never declared, instead of quietly answering "not there".

Four helpers do the check and the query in one step, so a factor rarely needs to write both:

| function | description |
| -------- | ----------- |
| `self:hasBonusOfType(present, type)` | whether the unit carries it at all |
| `self:getBonusValueOfType(unit, present, type)` | what every bonus of that type is worth together |
| `self:getBonusValueOfSubtype(unit, present, type, subtype)` | the same, narrowed to one subtype |
| `self:getBonusValueOfTypeAndRange(unit, present, type, shooting)` | the same, counting only what applies to this kind of blow |

Each answers 0 without asking the engine when the snapshot says the type is absent, which is the usual case. `present` is `info.attackerBonuses` or `info.defenderBonuses`, whichever unit is being asked about.

**Put the cheapest test first.** Conditions are evaluated left to right, so order them by what they cost:

```lua
-- good: a table read rules out almost every unit before anything is asked
if not self:hasBonusOfType(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end
if info.attacker:getCreature():getJsonKey() ~= "core:psychicElemental" then return 0 end
```

**Ask for a value rather than a list.** `getBonusesValue` returns what the matching bonuses are worth together, computed by the engine - one crossing. Fetching the list and adding up `getVal()` yourself crosses once for the list and once more for every bonus in it, and it also gets the answer wrong when bonuses do not simply add up (percentages, independent floors and ceilings).

```lua
-- good
local armour = info.defender:getBonusesValue({type = "GENERAL_DAMAGE_REDUCTION"})

-- bad: more crossings, and wrong for anything that is not plain addition
local list = info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION"})
local armour = 0
for i = 1, list:size() do armour = armour + list:getBonus(i):getVal() end
```

**Ask whether rather than which**, when the answer is all you need. `hasBonuses` says yes or no without the list ever being built for the script:

```lua
-- good
if info.defender:hasBonuses({type = "MIND_IMMUNITY"}) then ... end

-- bad: the whole list is handed over just to be counted
if info.defender:getBonuses({type = "MIND_IMMUNITY"}):size() > 0 then ... end
```

**Say as much as you can in the filter.** Type, subtype, source and the kind of blow are all matched by the engine, and a query the engine can describe is also a query it can cache. Only what the filter cannot express - "from anything except a spell" - belongs in a `filter` afterwards:

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

**`shooting` leaves out what does not count for this blow.** A bonus limited to melee is absent from a shot and the other way round, and one limited to neither always counts. Pass the flag of the attack straight through rather than reading `getEffectRange` yourself:

```lua
-- good
info.defender:getBonusesValue({type = "ENEMY_ATTACK_REDUCTION", shooting = info.shooting})
```

It asks for the kind of blow rather than for an effect range, because "counts in melee" is two effect ranges at once - and asking for them one at a time would add the two answers up instead of combining them the way the engine does.

**Do not build tables you do not need.** A factor that returns 0 for most attacks should return it before creating anything.

Everything else - arithmetic, comparisons, local variables - costs nothing worth thinking about. Write the calculation plainly; it is the questions asked of the engine that add up.
