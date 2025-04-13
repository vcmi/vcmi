# Bonus Format

## Full format

All parameters but type are optional.

```json
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

	// This sections allows to define 'limiter', which allows to limit bonus and only apply it under specific conditions
	// Typical conditions are "affect only specific creature", or "affect only if target has another, specific bonus"
	// See Bonus Limiters list below for full list of supported limiters 
	"limiters" : [
		"PREDEFINED_LIMITER", optional_parameters (...), //whhich one is preferred?
		{"type" : LIMITER_TYPE, "parameters" : [1,2,3]}
	],
	
	// Normally, only entities that are located "below" bonus source are affected by the bonus
	// For example, bonus on creature will only affect creature itself,
	// bonus on hero will affect hero itself and all its units, and player bonus will affect all objects owned by player
	// Propagator allows bonus to affect "upwards" entities. 
	// For example, creature that has bonus with battle-wide propagator will affect all units in combat, and not just unit itself
	// See Bonus Propagators list below for full list of supported propagators
	"propagator" : 	["PROPAGATOR_TYPE", optional_parameters (...)],
	
	// Updaters allow to modify bonus depending on context. 
	// For example, it can be used to scale bonus based on hero or unit level
	// See Bonus Updaters list below for full list of supported updaters
	"updater" :	    {Bonus Updater},
	
	// This is special type of propagator, that is only activated when bonus is being propagated upwards,
	// using its propagator. It has no effect on bonuses without propagator
	"propagationUpdater" :	{Bonus Updater, but works during propagation},
	
	// TODO
	"description" : "",
	
	// Stacking string allows to block stacking of bonuses from different entities
	// For example, devils and archdevils (different entities) both have battle-wide debuff to luck
	// Normally, having both such units in combat would result in bonus stacking, providing -2 debuff to luck in total
	// If such behavior is undesired, both such unit must have same, non-empty stacking string
	// String can contain any text, as long as it not empty and is same for both of such creatures
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

```json
"bonus" :
{
	"type" : "HATE",
	"subtype" : "creature.enchanter",
	"val" : 50
}
```

This bonus makes creature do 50% more damage to Enchanters.
