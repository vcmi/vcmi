# Creature Bank

Reward types for clearing creature bank are limited to resources, creatures, artifacts and spell.
Format of rewards is same as in [Rewardable Objects](Rewardable.md)

Deprecated in 1.6. Please use [Rewardable Objects](Rewardable.md) instead. See Conversion from 1.5 format section below for help with migration

### Example

This example defines a rewardable object with functionality similar of H3 creature bank.
See [Rewardable Objects](Rewardable.md) for detailed documentation of these properties.

```json
{
	"name" : "Cyclops Stockpile",

	// Generic message to ask player whether he wants to attack a creature bank, can be replaced with custom string
	"onGuardedMessage" : 32, 
	
	// Generic message to inform player that bank was already cleared
	"onVisitedMessage" : 33, 
	
	// As an alternative to a generic message you can define 'reward' 
	// that will be granted for visiting already cleared bank, such as morale debuff
	"onVisited" : [ 
		{
			"message" : 123, // "Such a despicable act reduces your army's morale."
			"bonuses" : [ { "type" : "MORALE", "val" : -1, "duration" : "ONE_BATTLE", "description" : 99 } ]
		}
	],
	"visitMode" : "once", // Banks never reset
	// Defines layout of guards. To emulate H3 logic, 
	// use 'creatureBankNarrow' if guardian units are narrow (1-tile units)
	// or, 'creatureBankWide' if defenders are double-hex units
	"guardsLayout" : "creatureBankNarrow",
	"rewards" : [
		{
			"message" : 34,
			"appearChance" : { "min" : 0, "max" : 30 },
			"guards" : [
				{ "amount" : 4, "type" : "cyclop" },
				{ "amount" : 4, "type" : "cyclop" },
				{ "amount" : 4, "type" : "cyclop", "upgradeChance" : 50 },
				{ "amount" : 4, "type" : "cyclop" },
				{ "amount" : 4, "type" : "cyclop" }
			],
			"resources" : {
				"gold" : 4000
			}
		},
		{
			"message" : 34,
			"appearChance" : { "min" : 30, "max" : 60 },
			"guards" : [
				{ "amount" : 6, "type" : "cyclop" },
				{ "amount" : 6, "type" : "cyclop" },
				{ "amount" : 6, "type" : "cyclop", "upgradeChance" : 50 },
				{ "amount" : 6, "type" : "cyclop" },
				{ "amount" : 6, "type" : "cyclop" }
			],
			"resources" : {
				"gold" : 6000
			}
		},
		{
			"message" : 34,
			"appearChance" : { "min" : 60, "max" : 90 },
			"guards" : [
				{ "amount" : 8, "type" : "cyclop" },
				{ "amount" : 8, "type" : "cyclop" },
				{ "amount" : 8, "type" : "cyclop", "upgradeChance" : 50 },
				{ "amount" : 8, "type" : "cyclop" },
				{ "amount" : 8, "type" : "cyclop" }
			],
			"resources" : {
				"gold" : 8000
			}
		},
		{
			"message" : 34,
			"appearChance" : { "min" : 90, "max" : 100 },
			"guards" : [
				{ "amount" : 10, "type" : "cyclop" },
				{ "amount" : 10, "type" : "cyclop" },
				{ "amount" : 10, "type" : "cyclop", "upgradeChance" : 50 },
				{ "amount" : 10, "type" : "cyclop" },
				{ "amount" : 10, "type" : "cyclop" }
			],
			"resources" : {
				"gold" : 10000
			}
		}
	]
},
```

### Conversion from 1.5 format

This is a list of changes that needs to be done to bank config to migrate it to 1.6 system. See [Rewardable Objects](Rewardable.md) documentation for description of new fields

- If your object type has defined `handler`, change its value from `bank` to `configurable`

- If your object has non-zero `resetDuration`, replace with `resetParameters` entry

- For each possible level, replace `chance` with `appearChance` entry

- If you have `combat_value` or `field` entries inside 'reward' - remove them. These fields are unused in both 1.5 and in 1.6

- Rename `levels` entry to `rewards`

- Add property `"visitMode" : "once"`
- Add property `"onGuardedMessage" : 119`, optionally - replace with custom message for object visit
- Add property `"onVisitedMessage" : 33`, optionally - custom message or morale debuff
- Add property `"message" : 34`, to every level of your reward, optionally - replace with custom message

### Old format (1.5 or earlier)

```json
{
	/// If true, battle setup will be like normal - Attacking player on the left, enemy on the right
	"regularUnitPlacement" : true,
	/// If true, bank placed on water will be visitable from coast (Shipwreck)
	"coastVisitable" : true,
	/// List of levels of this bank. On map loading, one of them will be randomly assigned to bank.
	"levels": [
		{
			/// Chance for this level to be active
			"chance": 30,

			/// Description of guards, stacks will be ordered
			/// on battlefield according to this scheme:
			/// 4    7    1
			/// 
			/// 6         5
			/// 
			/// 3         2
			/// Possible fields:
			/// amount - size of stack
			/// type - string ID of creature for this stack
			/// upgradeChance - chance (in percent) for this stack to be upgraded
			"guards": [
				{ "amount": 4, "type": "cyclop" },
				{ "amount": 4, "type": "cyclop" },
				{ "amount": 4, "type": "cyclop", "upgradeChance": 50 },
				{ "amount": 4, "type": "cyclop" },
				{ "amount": 4, "type": "cyclop" }
			],

			/// Description of rewards granted for clearing bank
			"reward" : {

				/// Granted resources
				"resources": {
					"wood" : 4,
					"mercury" : 4,
					"ore" : 4,
					"sulfur" : 4,
					"crystal" : 4,
					"gems" : 4,
					"gold" : 0
				},

				/// Granted creatures, same format as guards
				"creatures" : [
					{ "amount": 4, "type": "wyvern" }
				],

				/// List of random artifacts
				"artifacts": [ { "class" : "TREASURE" } ]

				/// List of spells
				"spells" : [ { "level" : 5 } ]
			} 
		}
	]
}

```
