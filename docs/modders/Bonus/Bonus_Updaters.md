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
    "valPer20" : 6,
    "stepSize" : 2
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

Effect: Updates val to `val = clamp(val * floor(stackSize / stepSize), minimum, maximum)`, where stackSize is total number of creatures in current unit stack

Parameters `minimum` and `maximum` are optional and can be dropped if not needed

Example:

```json
"updater" : {
    "type" : "TIMES_STACK_SIZE",
    "minimum" : 0,
    "maximum" : 100,
    "stepSize" : 2
}
```

## BONUS_OWNER_UPDATER

Helper updater for proper functionality of `OPPOSITE_SIDE` limiter
