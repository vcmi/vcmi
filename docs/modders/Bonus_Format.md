Enumerative parameters are described in HeroBonus.h file.

## Full format

All parameters but type are optional.

``` javascript
{
	"type":         "BONUS_TYPE",
	"subtype":      0,
	"val" :         0,
	"valueType":    "VALUE_TYPE",
	"addInfo" :     0, // or [1, 2, ...]

	"duration" :    "BONUS_DURATION", //or ["BONUS_DURATION1", "BONUS_DURATION2", ...]"
	"turns" :       0,

	"sourceType" :  "SOURCE_TYPE",
	"sourceID" :    0,
	"effectRange" : "EFFECT_RANGE",

	"limiters" : [
		"PREDEFINED_LIMITER", optional_parameters (...), //whhich one is preferred?
		{"type" : LIMITER_TYPE, "parameters" : [1,2,3]}
	],
	
	"propagator" : 	["PROPAGATOR_TYPE", optional_parameters (...)],
	"updater" :	    {Bonus Updater},
	"propagationUpdater" :	{Bonus Updater, but works during propagation},
	"description" : "",
	"stacking" :    ""
}
```

## Supported bonus types

- [Bonus Duration Types](Bonus/Bonus_Duration_Types.md)
- [Bonus Sources](Bonus/Bonus_Sources.md)
- [Bonus Limiters](Bonus/Bonus_Limiters.md)
- [Bonus Types](Bonus/Bonus_Types.md)
- [Bonus Propagators](Bonus/Bonus_Propagators.md)
- [Bonus Updaters](Bonus/Bonus_Updaters.md)
- [Bonus Range Types](Bonus/Bonus_Range_Types.md)
- [Bonus Value Types](Bonus/Bonus_Value_Types.md)

## Subtype resolution

All string identifiers of items can be used in "subtype" field. This
allows cross-referencing between the mods and make config file more
readable.

### Available prefixes

-   creature.
-   artifact.
-   skill:
``` javascript
		"pathfinding",  "archery",      "logistics",    "scouting",     "diplomacy",
		"navigation",   "leadership",   "wisdom",       "mysticism",    "luck",
		"ballistics",   "eagleEye",     "necromancy",   "estates",      "fireMagic",
		"airMagic",     "waterMagic",   "earthMagic",   "scholar",      "tactics",
		"artillery",    "learning",     "offence",      "armorer",      "intelligence",
		"sorcery",      "resistance",   "firstAid"
```

-   resource:
``` javascript
		"wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold", "mithril"
```

-   hero.
-   faction.
-   spell.
-   primarySkill
``` javascript
 "attack", "defence", "spellpower", "knowledge" 
```

-   terrain:
``` javascript
 "dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
```

### Example

``` javascript
"bonus" :
{
	"type" : "HATE",
	"subtype" : "creature.enchanter",
	"val" : 50
}
```

This bonus makes creature do 50% more damage to Enchanters.