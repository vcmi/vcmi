Artifact bonuses use [Bonus Format](../Bonus_Format.md)

TODO:

-   Artifacts growing with Commander level

## Required data

In order to make functional artifact you also need:

-   Icon for hero inventory (1 image)
-   Icon for popup windows (1 image, optional)
-   Animation for adventure map (1 animation)

## Format

``` javascript
{
	//what kind of bearer can use this artifact
	"type": ["HERO", "CREATURE", "COMMANDER"] 
	
	//TREASURE, MINOR, MAJOR, RELIC, SPECIAL
	"class": "TREASURE",
	
	//SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, RIGHT_RING, LEFT_RING, FEET, MISC1, MISC2, MISC3, MISC4,
	//MACH1, MACH2, MACH3, MACH4, SPELLBOOK, MISC5
	//also possible MISC, RING 
	"slot":	"HEAD", 

	//based on ARTRAITS.txt		
	"value": 12000, 

	"text":
	{
		"name": "Big Sword",
		"description": "Big sword gived +10 attack to hero",
		"event": "On your travel, you stumble upon big sword. You dust it off and stick in your backpack"
	},
	"graphics":
	{
		"image": "BigSword.png",
		"large": "BigSword_large.png",
		//def file for adventure map
		"map": "BigSword.def"
	},
	"bonuses":
	{
		Bonus_1,
		Bonus_2
	},
	
	//optional, for combined artifacts only
	"components": 
	[
		"artifact1",
		"artifact2",
		"artifact3"
	],
	
	//if set with artifact works like war machine
	"warMachine" : "some.creature" 
}
```