# Advanced Class Format

Advanced classes define an optional four-tier hero progression supplied entirely by mods.

- `name` - translatable class name.
- `description` - optional translatable description shown in the progression UI. If omitted, the UI lists configured bonuses and the battle ability.
- `tier` - progression tier from 1 to 4.
- `requiredLevel` - minimum hero level.
- `requiredPoints` - minimum score calculated from the tier-one class skill groups.
- `parents` - identifiers of classes from the previous tier that unlock this class; tier-one classes use an empty list. All parents must belong to the same tier-one path.
- `skillGroups` - lists of secondary-skill weights for basic, advanced, and expert mastery; only tier-one classes require this field. The lowest group score is used, which allows hybrid classes to require investment in multiple groups.
- `bonuses` - permanent bonuses granted when the class is selected, in [bonus format](../Bonus_Format.md).
- `ability` - optional tier-two or tier-three battle ability with a `spell` identifier and cooldown in rounds. Tier two uses the first ability slot and tier three the second one.

```json
{
	"squire" : {
		"name" : "Squire",
		"tier" : 1,
		"requiredLevel" : 10,
		"requiredPoints" : 10,
		"parents" : [],
		"skillGroups" : [
			{
				"offence" : [ 3, 4, 5 ],
				"armorer" : [ 3, 4, 5 ]
			}
		],
		"bonuses" : {
			"morale" : { "type" : "MORALE", "val" : 1 }
		}
	},
	"footman" : {
		"name" : "Footman",
		"tier" : 2,
		"requiredLevel" : 15,
		"requiredPoints" : 15,
		"parents" : [ "squire" ],
		"ability" : { "spell" : "stoneSkin", "cooldown" : 2 }
	}
}
```
