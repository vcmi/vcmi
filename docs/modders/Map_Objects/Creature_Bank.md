< [Documentation](../../Readme.md) / [Modding](../Readme.md) / [Map Object Format](../Map_Object_Format.md) / Creature Bank

This is description of fields for creature banks, part of [Object
Format](Object_Format "wikilink")

``` javascript
{
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

			// How hard are guards of this level. Unused?
			"combat_value": 506,

			/// Description of rewards granted for clearing bank
			"reward" : {

				/// Approximate value of reward, known to AI. Unused?
				"value": 10000,

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
		}
	]
}
```