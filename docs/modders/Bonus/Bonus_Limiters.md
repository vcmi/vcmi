# Bonus Limiters

## Predefined Limiters

The limiters take no parameters:

- SHOOTER_ONLY
- DRAGON_NATURE
- IS_UNDEAD
- CREATURE_NATIVE_TERRAIN
- CREATURE_FACTION
- SAME_FACTION
- CREATURES_ONLY
- OPPOSITE_SIDE

Example:

```json
"limiters" : [ "SHOOTER_ONLY" ]
```

## Customizable Limiters

### HAS_ANOTHER_BONUS_LIMITER

Bonus is only active if affected entity has another bonus that meets conditions

Parameters:

- Bonus type
- Bonus subtype (only used if bonus type is set)
- Bonus source type and bonus source ID

All parameters are optional. Values that don't need checking can be replaces with `null`

Examples:

- Adele specialty: active if unit has any bonus from Bless spell

```json
	"limiters" : [
		{
			"type" : "HAS_ANOTHER_BONUS_LIMITER",
			"parameters" : [
				null, // bonus type is ignored
				null, // bonus subtype is also ignored
				{
					"type" : "SPELL_EFFECT", // look for bonus of type SPELL_EFFECT
					"id" : "spell.bless"     // ... from spell "Bless"
				}
				]
		}
	],
```

- Mutare specialty: active if unit has `DRAGON_NATURE` bonus

```json
	"limiters" : [
		{
			"parameters" : [ "DRAGON_NATURE" ],
			"type" : "HAS_ANOTHER_BONUS_LIMITER"
		}
	],
```

### CREATURE_TYPE_LIMITER

Bonus is only active on creatures of specified type

Parameters:

- Creature id (string)
- (optional) include upgrades - default is false. If creature has multiple upgrades, or upgrades have their own upgrades, all such creatures will be affected. Special upgrades such as upgrades via specialties (Dragon, Gelu) are not affected

Example:

```json
"limiters": [ {
	"type":"CREATURE_TYPE_LIMITER",
	"parameters": [ "angel", true ]
} ],
```

### CREATURE_ALIGNMENT_LIMITER

Bonus is only active on creatures of factions of specified alignment

Parameters:

- Alignment identifier, `good`, `evil`, or `neutral`

### CREATURE_LEVEL_LIMITER

If parameters is empty, level limiter works as CREATURES_ONLY limiter

Parameters:

- Minimal level
- Maximal level

### FACTION_LIMITER

Parameters:

- Faction identifier

### CREATURE_TERRAIN_LIMITER

Parameters:

- Terrain identifier

Example:

```json
"limiters" : [ {
	"type" : "CREATURE_TERRAIN_LIMITER",
	"parameters" : ["sand"]
} ]
```

### UNIT_ON_HEXES

Parameters:

- List of affected battlefield hexes

For reference on tiles indexes see image below:

![Battlefield Hexes Layout](../../images/Battle_Field_Hexes.svg)

## Aggregate Limiters

The following limiters must be specified as the first element of a list,
and operate on the remaining limiters in that list:

- allOf (default when no aggregate limiter is specified)
- anyOf
- noneOf

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
