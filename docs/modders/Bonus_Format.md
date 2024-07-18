# Bonus Format

## Full format

All parameters but type are optional.

``` javascript
{
	// Type of the bonus. See Bonus Types for full list
	"type":         "BONUS_TYPE",

	// Subtype of the bonus. Function depends on bonus type.
	"subtype":      0,
	
	// Value of the bonus. Function depends on bonus type.
	"val" :         0,

	// Describes how this bonus is accumulated with other bonuses of the same type
	"valueType":    "VALUE_TYPE",
	
	// Additional info that bonus might need. Function depends on bonus type.
	"addInfo" :     0, // or [1, 2, ...]

	// How long this bonus should be active until removal.
	// May use multiple triggers, in which case first event will remove this bonus
	"duration" :    "BONUS_DURATION", //or ["BONUS_DURATION1", "BONUS_DURATION2", ...]"
	
	// How long this bonus should remain, in days or battle turns (depending on bonus duration)
	"turns" :       0,

	// TODO
	"targetSourceType" : "SOURCE_TYPE",
	
	// TODO
	"sourceType" :  "SOURCE_TYPE",
	
	// TODO
	"sourceID" :    0,
	
	// TODO
	"effectRange" : "EFFECT_RANGE",

	// TODO
	"limiters" : [
		"PREDEFINED_LIMITER", optional_parameters (...), //whhich one is preferred?
		{"type" : LIMITER_TYPE, "parameters" : [1,2,3]}
	],
	
	// TODO
	"propagator" : 	["PROPAGATOR_TYPE", optional_parameters (...)],
	
	// TODO
	"updater" :	    {Bonus Updater},
	
	// TODO
	"propagationUpdater" :	{Bonus Updater, but works during propagation},
	
	// TODO
	"description" : "",
	
	// TODO
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

All string identifiers of items can be used in "subtype" field. This allows cross-referencing between the mods and make config file more readable.
See [Game Identifiers](Game_Identifiers.md) for full list of available identifiers
 
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