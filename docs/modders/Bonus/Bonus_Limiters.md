# Bonus Limiters

## Predefined Limiters

Simple limiters that take no parameters.

Example:

```json
"limiters" : [ "SHOOTER_ONLY" ]
```

### SHOOTER_ONLY

Bonus is active if affected unit is a shooter (has SHOOTER bonus)

### DRAGON_NATURE

Bonus is active if affected unit is a dragon (has DRAGON_NATURE bonus)

### IS_UNDEAD

Bonus is active if affected unit is an undead (has UNDEAD bonus)

### CREATURE_NATIVE_TERRAIN

Bonus is active if affected unit is on native terrain

### CREATURES_ONLY

Bonus is active only on units

### OPPOSITE_SIDE

Bonus is active only for opposite side for a battle-wide bonus. Requires `BONUS_OWNER_UPDATER` to be present on bonus

## Customizable Limiters

### HAS_ANOTHER_BONUS_LIMITER

Bonus is only active if affected entity has another bonus that meets conditions

Parameters:

- `bonusType` - type of bonus to check against
- `bonusSubtype` - subtype of bonus to check against (only used if bonus type is set)
- `bonusSourceType` - source type of bonus to check against
- `bonusSourceID` -source ID of bonus to check against (only used if bonus type is set)

All parameters are optional.

Examples:

- Adele specialty: active if unit has any bonus from Bless spell

```json
	"limiters" : [
		{
			"type" : "HAS_ANOTHER_BONUS_LIMITER",
			"bonusSourceType" : "SPELL_EFFECT", // look for bonus of type SPELL_EFFECT
			"bonusSourceID" : "spell.bless"     // ... from spell "Bless"
		}
	],
```

- Mutare specialty: active if unit has `DRAGON_NATURE` bonus

```json
	"limiters" : [
		{
			"type" : "HAS_ANOTHER_BONUS_LIMITER",
			"bonusType" : "DRAGON_NATURE"
		}
	],
```

### CREATURE_TYPE_LIMITER

Bonus is only active on creatures of specified type

Parameters:

- `creature` - ID of creature to check against
- `includeUpgrades` - whether creature that is upgrade of `creature` should also pass this limiter. If creature has multiple upgrades, or upgrades have their own upgrades, all such creatures will be affected. Special upgrades such as upgrades via specialties (Dragon, Gelu) are not affected

Example:

```json
"limiters": [
	{
		"type" : "CREATURE_TYPE_LIMITER",
		"creature" : "angel",
		"includeUpgrades" : true
	}
],
```

### CREATURE_ALIGNMENT_LIMITER

Bonus is only active on creatures of factions of specified alignment

Parameters:

- `alignment` - ID of alignment that creature must have, `good`, `evil`, or `neutral`

```json
"limiters": [
	{
		"type" : "CREATURE_ALIGNMENT_LIMITER",
		"alignment" : "evil"
	}
],
```

### CREATURE_LEVEL_LIMITER

If parameters is empty, any creature will pass this limiter

Parameters:

- `minLevel` - minimal level that creature must have to pass limiter
- `maxlevel` - maximal level that creature must have to pass limiter

```json
"limiters": [
	{
		"type" : "CREATURE_LEVEL_LIMITER",
		"minLevel" : 1,
		"maxlevel" : 5
	}
],
```

### FACTION_LIMITER

Also available as `CREATURE_FACTION_LIMITER`

Parameters:

- `faction` - faction that creature or hero must belong to

```json
"limiters": [
	{
		"type" : "FACTION_LIMITER",
		"faction" : "castle"
	}
],
```

### TERRAIN_LIMITER

Also available as `CREATURE_TERRAIN_LIMITER`

Parameters:

- `terrain` - identifier of terrain on which this creature, hero or town must be to pass this limiter. If not set, creature will pass this limiter if it is on native terrain

The native terrain of a town or a hero depends on their factions (e.g for a castle and a knight it is grass). The terrain type occupied by a town depends on its entrance tile (the tile with flags that is taken by visiting heros).

Example:

```json
"limiters" : [
	{
		"type" : "TERRAIN_LIMITER",
		"terrain" : "sand"
	}
]
```

### HAS_CHARGES_LIMITER

Currently works only with spells. Sets the cost of use in charges

Parameters:

- `cost` - use cost (charges)

```json
"limiters" : [
	{
		"type" : "HAS_CHARGES_LIMITER",
		"cost" : 2
	}
]
```

### UNIT_ON_HEXES

Parameters:

- `hexes` - List of affected battlefield hexes

```json
"limiters" : [
	{
		"type" : "UNIT_ON_HEXES",
		"hexes" : [ 25, 50, 75 ]
	}
]
```

For reference on tiles indexes see image below:

![Battlefield Hexes Layout](../../images/Battle_Field_Hexes.svg)

## Aggregate Limiters

The following limiters must be specified as the first element of a list, and operate on the remaining limiters in that list:

- `allOf` (default when no aggregate limiter is specified)
- `anyOf`
- `noneOf`

Example:

```json
"limiters" : [
    "noneOf",
    "IS_UNDEAD",
    {
        "type" : "HAS_ANOTHER_BONUS_LIMITER",
        "parameters" : [ "SIEGE_WEAPON" ]
    }
]
```

## Propagating Limiters

When bonuses with limiters are propagated limiters are applied independently on each node. E.g. a bonus with "HAS_ANOTHER_BONUS_LIMITER" propagated to a hero from a stack will apply to the hero only if the hero has the required bonus, independently of the stack bonuses.
