< [Documentation](../../Readme.md) / [Modding](../Readme.md) / Entities Format / Hero Type Format

## Required data

In order to make functional hero you also need:

-   Portraits, small and big versions (2 images)
-   Specialty icons, small and big versions (2 images)

## Format

``` javascript
"myHeroName" :
{
	// Identifier of class. Usually camelCase version of human-readable name
	"class" : "wizard",

	// List of starting spells, if available. Will also grant spellbook
	"spellbook" :
	[
		"magicArrow"
	],

	// Set to true if the hero is female by default (can be changed in map editor)
	"female" : true,

	// If set to true hero will be unavailable on start and won't appear in taverns (campaign heroes)
	"special" : true,

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
			"tooltip" : "{Magic Arrow}\n\nCasts powerfull magic arrows",

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

		// Class-independent animation in battle
		"small" : "myMod/myHero/battle.def"

	},

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

	// List of skills received by hero
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
		// to be merged with all bonuses, use for specialties with multiple similar bonuses (optional)
		"base" : {common bonus properties},
		"bonuses" : {
			// use updaters for bonuses that grow with level
			"someBonus" : {Bonus Format},
			"anotherOne" : {Bonus Format}
		},
		// adds creature specialty following the HMM3 default formula
		"creature" : "griffin"
	}
}
```