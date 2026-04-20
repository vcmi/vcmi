# Bonus Updaters

Updaters come in two forms: simple and complex. Simple updaters take no
parameters and are specified as strings. Complex updaters do take
parameters (sometimes optional), and are specified as structs.

Check the files in `config/heroes/` for additional usage examples.

## GROWS_WITH_LEVEL

Effect: Updates val to `ceil(valPer20 * floor(heroLevel / stepSize) / 20)`

Example: The following updater will cause a bonus to grow by 6 for every 40 levels. At first level, rounding will cause the bonus to be 0.

```json
"updater" : {
    "type" : "GROWS_WITH_LEVEL",
    "valPer20" : 6,
    "stepSize" : 2
}
```

Example: The following updater will cause a bonus to grow by 3 for every 20 levels. At first level, rounding will cause the bonus to be 1.

```json
"updater" : {
    "type" : "GROWS_WITH_LEVEL",
    "valPer20" : 3,
    "stepSize" : 1
}
```

Remarks:

- The rounding rules are designed to match the attack/defense bonus progression for heroes with creature specialties in HMM3.
- There is no point in specifying val for a bonus with a GROWS_WITH_LEVEL updater.

## TIMES_HERO_LEVEL

Effect: Updates val to `val * heroLevel / stepSize`

Usage: `"updater" : "TIMES_HERO_LEVEL"`

Usage with stepSize greater than one:

```json
"updater" : {
    "type" : "TIMES_HERO_LEVEL",
    "stepSize" : 2
}
```

## TIMES_STACK_LEVEL

Updates val to `val * stackLevel`, where `stackLevel` is level of stack (Pikeman is level 1, Angel is level 7)

Usage:

`"updater" : "TIMES_STACK_LEVEL"`

Remark: The stack level for war machines is 0.

## DIVIDE_STACK_LEVEL

Updates val to `val / stackLevel`, where `stackLevel` is level of stack (Pikeman is level 1, Angel is level 7)

Usage:

`"updater" : "DIVIDE_STACK_LEVEL"`

Remark: The stack level for war machines is 0.

## TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL

Effect: Same effect as `TIMES_HERO_LEVEL` combined with `DIVIDE_STACK_LEVEL`: `val * heroLevel / stackLevel`

Intended to be used as hero bonus (such as specialty, skill, or artifact), for bonuses that affect units (Example: Adela Bless specialty)

Usage:

`"updater" : "TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL"`

## TIMES_STACK_SIZE

Effect: Updates val to `val = stepValue * clamp(val * floor(stackSize / stepSize), minimum, maximum)`, where stackSize is total number of creatures in current unit stack.

Example of short form with default parameters:

```json
"updater" : "TIMES_STACK_SIZE"
```

Example of long form with custom parameters:

```json
"updater" : {
    "type" : "TIMES_STACK_SIZE",
    
    // Optional, by default - unlimited
    "minimum" : 0,
    
    // Optional, by default - unlimited
    "maximum" : 100,
    
    // Optional, by default - 1
    "stepSize" : 2
    // Optional, by default - 1
    "stepValue" : 2
}
```

## TIMES_ARMY_SIZE

Effect: Updates val to `val = clamp(val * floor(stackSize / stepSize), minimum, maximum)`, where stackSize is total number of creatures in hero army that fulful filter

Parameters:

- `minimum`: minimum possible value of the bonus value. Unlimited by default
- `maximum`: maximum possible value of the bonus value. Unlimited by default
- `stepSize`: number of units needed to increase updater multiplier by 1
- `filteredCreature`: identifier of specific unit to filter
- `filteredLevel`: level of units that need to be counted. Redundant if `filteredCreature` is used
- `filteredFaction`: faction of units that need to be counted. Redundant if `filteredCreature` is used

Filtering for specific unit:

```json
"updater" : {
    "type" : "TIMES_ARMY_SIZE",
    "filteredCreature" : "pikeman",
    
    // Optional, by default - unlimited
    "minimum" : 0,
    
    // Optional, by default - unlimited
    "maximum" : 100,
    
    // Optional, by default - 1
    "stepSize" : 2
}
```

Filtering for specific faction:

```json
"updater" : {
    "type" : "TIMES_STACK_SIZE",
    "filteredFaction" : "castle"
}
```

Filtering for specific unit level:

```json
"updater" : {
    "type" : "TIMES_STACK_SIZE",
    "filteredLevel" : 2
}
```

## TIMES_SIDE_BATTLE_SPELLS_CAST

Effect: Updates val to `val * clamp(floor(spellCasts / stepSize), minimum, maximum)`, where `spellCasts` is the number of completed spells already cast by the bonus owner's side in the current battle, filtered by `casterType`.

This updater only has effect when the bonus is evaluated on a battle hero or battle stack. Outside battle it returns the original bonus unchanged, so use an `IN_BATTLE` limiter when the bonus should only work during combat.

Parameters:

- `casterType` - spell cast source to count. Supported values are `hero`, `creature`, and `any`. Default is `hero`
- `minimum` - lower bound for the multiplier after dividing by `stepSize`. Default is `0`
- `maximum` - upper bound for the multiplier after dividing by `stepSize`. Default is unlimited
- `stepSize` - number of completed spells required for each multiplier step. Default is `1`

Example:

```json
"updater" : {
    "type" : "TIMES_SIDE_BATTLE_SPELLS_CAST",
    "casterType" : "hero",
    "maximum" : 5
},
"limiters" : [ "IN_BATTLE" ]
```

The example above turns a bonus with `val : 1` into `+1` of that bonus's value per previously cast hero spell on the same side, capped at 5.

## BONUS_OWNER_UPDATER

Helper updater for proper functionality of `OPPOSITE_SIDE` limiter
