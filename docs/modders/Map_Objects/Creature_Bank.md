# Creature Bank

Reward types for clearing creature bank are limited to resources, creatures, artifacts and spell.
Format of rewards is same as in [Rewardable Objects](Rewardable.md)

``` javascript
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