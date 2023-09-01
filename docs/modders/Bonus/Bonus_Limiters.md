## Predefined Limiters

The limiters take no parameters:

-   SHOOTER_ONLY
-   DRAGON_NATURE
-   IS_UNDEAD
-   CREATURE_NATIVE_TERRAIN
-   OPPOSITE_SIDE

Example:

``` javascript
"limiters" : [ "SHOOTER_ONLY" ]
```

## Customizable Limiters

### HAS_ANOTHER_BONUS_LIMITER

Parameters:

-   Bonus type
-   (optional) bonus subtype
-   (optional) bonus sourceType and sourceId in struct
-   example: (from Adele's bless):

``` javascript
	"limiters" : [
		{
			"type" : "HAS_ANOTHER_BONUS_LIMITER",
			"parameters" : [
				"GENERAL_DAMAGE_PREMY",
				1,
				{
					"type" : "SPELL_EFFECT",
					"id" : "spell.bless"
				}
				]
		}
	],
```

### CREATURE_TYPE_LIMITER

Parameters:

-   Creature id (string)
-   (optional) include upgrades - default is false

### CREATURE_ALIGNMENT_LIMITER

Parameters:

-   Alignment identifier

### CREATURE_FACTION_LIMITER

Parameters:

-   Faction identifier

### CREATURE_TERRAIN_LIMITER

Parameters:

-   Terrain identifier

Example:

``` javascript
"limiters": [ {
	"type":"CREATURE_TYPE_LIMITER",
	"parameters": [ "angel", true ]
} ],
```

``` javascript
"limiters" : [ {
	"type" : "CREATURE_TERRAIN_LIMITER",
	"parameters" : ["sand"]
} ]
```

## Aggregate Limiters

The following limiters must be specified as the first element of a list,
and operate on the remaining limiters in that list:

-   allOf (default when no aggregate limiter is specified)
-   anyOf
-   noneOf

Example:

``` javascript
"limiters" : [
    "noneOf",
    "IS_UNDEAD",
    {
        "type" : "HAS_ANOTHER_BONUS_LIMITER",
        "parameters" : [ "SIEGE_WEAPON" ]
    }
]
```