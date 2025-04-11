# Bonus Updaters

TODO: this page may be incorrect or outdated

Updaters come in two forms: simple and complex. Simple updaters take no
parameters and are specified as strings. Complex updaters do take
parameters (sometimes optional), and are specified as structs.

Check the files in *config/heroes/* for additional usage examples.

## GROWS_WITH_LEVEL

- Type: Complex
- Parameters: valPer20, stepSize=1
- Effect: Updates val to `ceil(valPer20 * floor(heroLevel / stepSize) / 20)`

Example: The following updater will cause a bonus to grow by 6 for every
40 levels. At first level, rounding will cause the bonus to be 0.

```json
"updater" : {
    "parameters" : [ 6, 2 ],
    "type" : "GROWS_WITH_LEVEL"
}
```

Example: The following updater will cause a bonus to grow by 3 for every
20 levels. At first level, rounding will cause the bonus to be 1.

```json
"updater" : {
    "parameters" : [ 3 ],
    "type" : "GROWS_WITH_LEVEL"
}
```

Remarks:

- The rounding rules are designed to match the attack/defense bonus
    progression for heroes with creature specialties in HMM3.
- There is no point in specifying val for a bonus with a
    GROWS_WITH_LEVEL updater.

## TIMES_HERO_LEVEL

- Type: Simple
- Effect: Updates val to `val * heroLevel`

Usage: `"updater" : "TIMES_HERO_LEVEL"`

Remark: This updater is redundant, in the sense that GROWS_WITH_LEVEL
can also express the desired scaling by setting valPer20 to 20\*val. It
has been added for convenience.

## TIMES_STACK_LEVEL

- Type: Simple
- Effect: Updates val to `val * stackLevel`

Usage:

`"updater" : "TIMES_STACK_LEVEL"`

Remark: The stack level for war machines is 0.

## DIVIDE_STACK_LEVEL

- Type: Simple
- Effect: Updates val to `val / stackLevel`

Usage:

`"updater" : "DIVIDE_STACK_LEVEL"`

Remark: The stack level for war machines is 0.

## TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL

- Type: Simple
- Effect: Same effect as `TIMES_HERO_LEVEL` combined with `DIVIDE_STACK_LEVEL`: `val * heroLevel / stackLevel`

Intended to be used as hero bonus (such as specialty, skill, or artifact), for bonuses that affect units (Example: Adela Bless specialty)

Usage:

`"updater" : "TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL"`

## BONUS_OWNER_UPDATER

TODO: document me
