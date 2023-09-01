< [Documentation](../../Readme.md) / [Modding](../Readme.md) / Entities Format / Hero Class Format

## Required data

In order to make functional hero class you also need:

-   Adventure animation (1 def file)
-   Battle animation, male and female version (2 def files)

## Format

``` javascript
// Unique identifier of hero class, camelCase
"myClassName" :
{
	// Various hero animations
	"animation"
	{
		"battle" : 
		{
			// Battle animation for female heroes
			"female" : "myMod/battle/heroFemale",

			// Battle animation for male heroes, can be same as female
			"male"   : "myMod/battle/heroMale"
		}
	},

	// Description of map object representing this hero class. See map template format for details
	"mapObject" : {
		// Optional, hero ID-base filter, using same rules as building requirements
		"filters" : {
			"mutare" : [ "anyOf", [ "mutare" ], [ "mutareDrake" ]]
		},

		// List of templates used for this object, normally - only one is needed
		"templates" : {
			"normal" : { "animation" : "AH00_.def" }
		}
	},

	// Translatable name of hero class
	"name" : "My hero class",

	// Identifier of faction this class belongs to
	"faction" : "myFaction",

	// Identifier of creature that should be used as commander for this hero class
	// Can be a regular creature that has shooting animation
	"commander" : "mage",

	// Affinity of this class, might or magic
	"affinity" : "might",

	// Initial primary skills of heroes
	"primarySkills" :
	{
		"attack"     : 2,
		"defence"    : 0,
		"spellpower" : 1,
		"knowledge"  : 2
	},

	// Chance to get specific primary skill on level-up
	// This set specifies chances for levels 2-9
	"lowLevelChance" :
	{
		"attack"     : 15,
		"defence"    : 10,
		"spellpower" : 50,
		"knowledge"  : 25
	},

	// Chance to get specific primary skill on level-up
	// This set specifies chances for levels starting from 10
	"highLevelChance" :
	{
		"attack"     : 25,
		"defence"    : 5,
		"spellpower" : 45,
		"knowledge"  : 25
	},

	// Chance to get specific secondary skill on level-up
	// Skills not listed here will be considered as unavailable, including universities
	"secondarySkills" :
	{
		"pathfinding"  : 3.
		"archery"      : 6.
		 ...
		"resistance"   : 5,
		"firstAid"     : 4
	},

	// Chance for a this hero class to appear in a town, creates pair with same field in town format 
	// Used for situations where chance was not set in "tavern" field, chance will be determined as:
	// square root( town tavern chance * hero class tavern chance )
	"defaultTavern" : 5,

	// Chance for this hero to appear in tavern of this factions.
	// Reversed version of field "tavern" from town format
	// If faction-class pair is not listed in any of them
	// chance set to 0 and the class won't appear in tavern of this town
	"tavern" :
	{
		"castle"     : 4,
		 ...
		"conflux"    : 6
	}
}
```