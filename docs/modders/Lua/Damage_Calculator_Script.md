# Damage Calculator Script

Declared with `"implements" : "damageCalculator"` in the [scripts](Script_Types.md) section of a mod. This script decides what an attack is worth - the damage of the creatures, everything that raises or lowers it, and how many creatures die.

Unlike the other script types there is exactly **one** damage calculator in a game. It is not attached to a unit and nothing grants it: the engine asks it about every attack, whether the blow is being dealt or an AI is only weighing it. VCMI ships `core:damageCalculator`, and a mod changes the rules by [stacking a patch](#changing-the-rules) over it rather than by declaring one of its own.

```json
"damageCalculator" : {
    "implements" : "damageCalculator",
    "script" : "damage/damageCalculator",
    "patches" : [ "damage/turret", "damage/siegeWeapon", ... ],
    "schema" : { "properties" : {}, "additionalProperties" : false }
}
```

Sources of this type live in the `damage/` directory, and the rules VCMI itself keeps out of the base script are stacked over it as patches - which is also the worked example of how to write one.

`priority` and `description` are not used - nothing else reacts alongside it, and nothing shows it to the player.

## How damage is worked out

Every attack goes through the same three steps.

**1. Base damage.** What the creatures themselves deal, min and max, multiplied by how many of them are alive. Bless and curse collapse that range onto one of its ends, a ballista multiplies it by the attack of its hero.

**2. Factors.** Everything that changes the blow is a *factor* - a signed share of the base damage. **Positive raises it, negative lowers it.** What decides how a factor applies is its sign alone, not where it came from:

- factors that raise the damage **add up**: attack over defence (+5% per point), offence (+30%), luck (+100%) give `1 + 0.05×points + 0.3 + 1.0`
- factors that lower it **multiply**, each taking its share of what is left: armourer (-15%) and a shooting penalty (-50%) give `0.85 × 0.5`

The two totals are multiplied together. This is why a single -50% never quite halves the damage twice, and why giving a "boost" a negative value turns it into a mitigation rather than cancelling out other boosts.

**3. Casualties.** How many creatures the resulting damage kills, given the health left on the first one.

## Changing the rules

Write a patch, list it in `patches`, and override the one step you care about. `Base` is the script you are stacked over:

```lua
local Script = setmetatable({}, {__index = Base})
Script.__index = Script

--- Trolls hit twice as hard under a full moon.
function Script:getFactors(info)
    local factors = Base.getFactors(self, info)

    if isFullMoon() and info.attacker:getCreature():getJsonKey() == "myMod:troll" then
        table.insert(factors, 1.0)
    end

    return factors
end

return Script
```

Call up the chain with `Base.method(self, ...)` - a dot and an explicit `self`. Writing `self:method(...)` dispatches back into your own patch and loops forever.

Every step is a method and can be overridden the same way: `getBaseDamageSingle`, `getBaseDamageBlessCurse`, `getAttack`, `getDefense`, `getDamageCap`, `getCasualties`, or any single factor such as `getJoustingFactor`.

Some steps exist only to be patched. `getAttackIgnored` and `getDamageCap` answer "nothing" in the base script, because nothing in Heroes 3 lowers the attack of whoever strikes it or caps the damage a blow may deal - the rules that do live in `damage/enemyAttackReduction` and `damage/damageReceivedCap`. Read those two for the shortest example of a patch, and `damage/vulnerableFromBack` for one that adds a factor.

Each patch keeps to one rule. That is what lets a mod drop or replace a single one of them without touching anything else, and it is worth following in mod patches too.

## What the script is given

`Script:calculate(battle, info)` receives the battle and one table describing the attack:

- `attacker`, `defender` - the two units. See [Unit](API_Reference.md#unit)
- `attackerHex`, `defenderHex` - where the blow happens. Already resolved, so an attack that has not happened yet reads like any other. Pass them to `battle:hasDistancePenalty` and friends rather than asking the units where they stand
- `shooting`, `luckyStrike`, `unluckyStrike`, `deathBlow`, `doubleDamage` - what kind of blow this is
- `chargeDistance` - hexes crossed to reach the target, which is what jousting scales with
- `attackerBonuses`, `defenderBonuses` - which of the [declared bonus types](#declaring-what-you-look-at) each unit carries. Read them through `self:carriesBonus(info.attackerBonuses, "JOUSTING")`
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
function Script:bonusTypes()
    return { "JOUSTING", "HATE", "IN_FRENZY", ... }
end
```

A patch that adds a factor of its own **must add whatever it looks at**, or the check will not find it:

```lua
function Script:bonusTypes()
    local types = Base.bonusTypes(self)
    table.insert(types, "MYMOD_MOON_FURY")
    return types
end
```

Asking about a type that was never declared raises an error naming it, rather than quietly answering "not there" and costing damage.

## Writing a factor that does not slow the game down

This script runs on every attack the game resolves **and on every attack an AI considers** - some two hundred thousand times per AI turn in a large battle. A factor that is careless about it is felt as the AI thinking longer, not as a dropped frame, so it is worth knowing which lines are cheap and which are not.

**Reading `info` is free. Calling into the engine is not.** Anything reached through a `:` on a unit, a bonus or the battle crosses into the engine and back. Reading a field of `info`, or of the two bonus tables, is a plain table lookup.

**Check the bonus table before you ask anything.** This is the single most useful habit: most units carry none of what a given factor looks for, and the table answers that without leaving the script.

```lua
-- good: the query only happens for a unit that actually has the bonus
if not self:carriesBonus(info.attackerBonuses, "JOUSTING") then return 0 end

return info.chargeDistance * info.attacker:getBonusesValue({type = "JOUSTING"}) / 100
```

`carriesBonus` reads the same table you could read yourself - `info.attackerBonuses.JOUSTING` does the same job - but it also complains when the type was never declared, instead of quietly answering "not there".

**Put the cheapest test first.** Conditions are evaluated left to right, so order them by what they cost:

```lua
-- good: a table read rules out almost every unit before anything is asked
if not self:carriesBonus(info.defenderBonuses, "MIND_IMMUNITY") then return 0 end
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

**Say as much as you can in the filter.** Type, subtype and source are all matched by the engine, and a query the engine can describe is also a query it can cache. Only what the filter cannot express - "from anything except a spell", "whichever of these applies in melee" - belongs in a `filter` afterwards:

```lua
-- good: the engine finds them
info.defender:getBonusesValue({
    type = "GENERAL_DAMAGE_REDUCTION",
    subtype = "damageTypeAll",
    sourceType = ENUM.BonusSource.spellEffect
})

-- only when equality is not enough
info.defender:getBonuses({type = "GENERAL_DAMAGE_REDUCTION"}):filter(function(bonus)
    return bonus:getSource() ~= ENUM.BonusSource.spellEffect
end):totalValue()
```

**Do not build tables you do not need.** A factor that returns 0 for most attacks should return it before creating anything.

Everything else - arithmetic, comparisons, local variables - costs nothing worth thinking about. Write the calculation plainly; it is the questions asked of the engine that add up.
