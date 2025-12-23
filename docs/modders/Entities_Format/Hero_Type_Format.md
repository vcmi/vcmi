# Hero Type Format

## Required data

In order to make functional hero you also need:

- Portraits, small and big versions (2 images)
- Specialty icons, small and big versions (2 images)

## Format

```json
"myHeroName" :
{
	// Identifier of class this hero belongs to. Such as knight or battleMage
	"class" : "wizard",

	// List of starting spells, if available. This entry (even empty) will also grant spellbook
	"spellbook" :
	[
		"magicArrow"
	],

	// Set to true if the hero is female by default (can be changed in map editor)
	"female" : true,

	// If set to true hero will be unavailable on start and won't appear in taverns (campaign heroes)
	"special" : true,
	
	// If set to true, hero won't show up on a map with water
	"onlyOnWaterMap" : false,
	
	// If set to true, hero will show up only if the map contains no water
	"onlyOnMapWithoutWater" : false,

	// All translatable texts related to hero
	"texts" :
	{
		"name" : "My Hero",
		"biography" : "This is a long story...",

		"specialty" :
		{
			// Description visible when hovering over specialty icon
			"description" : "Spell mastery: Magic Arrow",

			// Tooltip visible on clicking icon. Can use {} symbols to change title to yellow
			// as well as escape sequences "\n" to add line breaks
			"tooltip" : "{Magic Arrow}\n\nCasts powerful magic arrows",

			// Name of your specialty
			"name" : "Magic Arrow"
		}
	},

	// Graphics used by hero
	"images" :
	{
		// Small 32px speciality icon
		"specialtySmall" : "myMod/myHero/specSmall.png",

		// Large 44px speciality icon
		"specialtyLarge" : "myMod/myHero/specLarge.png",

		// Large 58x64px portrait
		"large" : "myMod/myHero/large.png",

		// Small 48x32px portrait
		"small" : "myMod/myHero/small.png"
	},
	
	// Custom animation to be used on battle, overrides hero class property
	"battleImage" : "heroInBattle.def"

	// Initial hero army when recruited in tavern
	// Must have 1-3 elements
	"army" :
	[
		// First always available stack
		{
			// Identifier of creature in this stack
			"creature" : "mage",

			// Minimal and maximum size of stack. Size will be
			// determined randomly at the start of the game
			"max" : 2,
			"min" : 1
		},
		// Second stack has 90 % chance to appear
		{
			"creature" : "archmage",
			"max" : 1,
			"min" : 1
		},
		// Third stack with just 20 % chance to appear
		{
			"creature" : "mage",
			"max" : 2,
			"min" : 1
		}
	],

	// List of skills initially known by hero
	// Not limited by size - you can add as many skills as you wish
	"skills" :
	[
		{
			// Skill level, basic, advanced or expert
			"level" : "basic",

			// Skill identifier, camelCase version of name
			"skill" : "wisdom"
		},
		{
			"level" : "basic",
			"skill" : "waterMagic"
		}
	],

	// Description of specialty mechanics using bonuses (with updaters)
	"specialty" : {
	
		// Optional. Section that will be added into every bonus instance, for use in specialties with multiple similar bonuses
		"base" : {common bonus properties},
		
		// List of bonuses added by this specialty. See bonus format for more details
		"bonuses" : {
			// use updaters for bonuses that grow with level
			"someBonus" : {Bonus Format},
			"anotherOne" : {Bonus Format}
		},
		// Shortcut for defining creature specialty, using standard H3 rules
		// Can be combined with bonuses-based specialty if desired
		"creature" : "griffin",

		// Shortcut for defining specialty in secondary skill, using standard H3 rules
		// Can be combined with bonuses-based specialty if desired
		"secondary" : "offence",
		
		// Optional, only applicable to creature specialties
		// Overrides creature level to specific value for purposes of computing growth of h3-like creature specialty
		"creatureLevel" : 5
		
		// Optional, only applicable to creature and secondary skill specialties
		// Overrides default (5% for vanilla H3 specialties) growth of specialties per level to a specified value
		// Default value can be modified globally using specialtySecondarySkillGrowth and specialtyCreatureGrowth game settings
		"stepSize" : 5
	}
}
```
