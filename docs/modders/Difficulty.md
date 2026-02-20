# Difficulty

Since VCMI 1.4.0 there are more capabilities to configure difficulty parameters.
It means, that modders can give different bonuses to AI or human players depending on selected difficulty

Difficulty configuration is located in [config/difficulty.json](../../config/difficulty.json) file and can be overridden by mods.

## Format summary

```json
{
	"human": //parameters impacting human players only
	{
		"pawn": //parameters for specific difficulty
		{
			//starting resources
			"resources": { "wood" : 30, "mercury": 15, "ore": 30, "sulfur": 15, "crystal": 15, "gems": 15, "gold": 30000 },
			//bonuses will be given to player globally
			"globalBonuses": [],
			//bonuses will be given to player every battle
			"battleBonuses": []
		},
		"knight": {},
		"rook": {},
		"queen": {},
		"king": {},
	},
	"ai": //parameters impacting AI players only
	{
		"pawn": {}, //parameters for specific difficulty 
		"knight": {},
		"rook": {},
		"queen": {},
		"king": {},
	}
}
```

## Bonuses

It's possible to specify bonuses of two types: `globalBonuses` and `battleBonuses`.

Both are arrays containing any amount of bonuses, each can be described as usual bonus. See details in [bonus documentation](Bonus_Format.md).

`globalBonuses` are given to player on the beginning and depending on bonus configuration, it can behave diffierently.

`battleBonuses` are given to player during the battles, but *only for battles with neutral forces*. So it won't be provided to player for PvP battles and battles versus AI heroes/castles/garrisons. To avoid cumulative effects or unexpected behavior it's recommended to specify bonus `duration` as `ONE_BATTLE`.

For both types of bonuses, `source` should be specified as `OTHER`.

## Example

```json
{ //will give 150% extra health to all players' creatures if specified in "battleBonuses" array
	"type" : "STACK_HEALTH",
	"val" : 150,
	"valueType" : "PERCENT_TO_ALL",
	"duration" : "ONE_BATTLE",
	"sourceType" : "OTHER"
},
```

## Compatibility

Starting from VCMI 1.4 `startres.json` is not available anymore and will be ignored if present in any mod.
Thus, `Resourceful AI`  mod of version 1.2 won't work anymore.
