# Artifact Format

Artifact bonuses use [Bonus Format](../Bonus_Format.md)

## Required data

In order to make functional artifact you also need:

-   Icon for hero inventory (1 image)
-   Icon for popup windows (1 image, optional)
-   Animation for adventure map (1 animation)

## Format

``` jsonc
{
	// Type of this artifact - creature, hero or commander
	"type": ["HERO", "CREATURE", "COMMANDER"] 
	
	// TREASURE, MINOR, MAJOR, RELIC, SPECIAL
	"class": "TREASURE",
	
	// Slot(s) to which this artifact can be put, if applicable
	// SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, RIGHT_RING, LEFT_RING, FEET, MISC1, MISC2, MISC3, MISC4,
	// MACH1, MACH2, MACH3, MACH4, SPELLBOOK, MISC5
	// MISC, RING 
	"slot":	"HEAD", 
	"slot":	[ "LEFT_HAND", "RIGHT_HAND ],

	// Cost of this artifact, in gold
	"value": 12000, 

	"text":
	{
		// Name of the artifact
		"name": "Big Sword",
		
		// Long description of this artifact
		"description": "Big sword gived +10 attack to hero",
		
		// Text that visible on picking this artifact on map
		"event": "On your travel, you stumble upon big sword. You dust it off and stick in your backpack"
	},
	"graphics":
	{
		// Base image for this artifact, used for example in hero screen
		"image": "BigSword.png",

		// Large image, used for drag-and-drop and popup messages
		"large": "BigSword_large.png",

		//def file for adventure map
		"map": "BigSword.def"
	},

	// Bonuses provided by this artifact using bonus system
	"bonuses":
	{
		Bonus_1,
		Bonus_2
	},

	// Optional, list of components for combinational artifacts
	"components": 
	[
		"artifact1",
		"artifact2",
		"artifact3"
	],
	
	// Optional, by default is false. Set to true if components are supposed to be fused. 
	"fusedComponents" : true,

	// Creature id to use on battle field. If set, this artifact is war machine
	"warMachine" : "some.creature" 

	// If set to true, artifact won't spawn on a map without water
	"onlyOnWaterMap" : false,

	// TODO: document
	"growing" : {
		"bonusesPerLevel" : {},
		"thresholdBonuses" : {},
	}
}
```
