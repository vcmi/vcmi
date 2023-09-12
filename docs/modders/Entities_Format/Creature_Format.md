< [Documentation](../../Readme.md) / [Modding](../Readme.md) / Entities Format / Creature Format

## Required data

In order to make functional creature you also need:

### Animation

-   Battle animation (1 def file)
-   Set of rendered projectiles (1 def files, shooters only)
-   Adventure map animation (1 def file)

### Images

-   Small portrait for hero exchange window (1 image)
-   Large portrait for hero window (1 image)

### Sounds

-   Set of sounds (up to 8 sounds)

## Format

``` javascript
// camelCase unique creature identifier
"creatureName" : 
{
	// translatable names
	"name" :
	{
		"singular" : "Creature",
		"plural" : "Creatures"
	},
	"level" : 0,

	// if set to true creature will not appear in-game randomly (e.g. as neutral creature)
	"special" : true, 
	
	// config name of faction. Examples: castle, rampart
	"faction" : "", 
	// cost to recruit, zero values can be omitted.
	"cost" : 
	{
		"wood" : 0,
		"mercury" : 0,
		"ore" : 0,
		"sulfur" : 0,
		"crystal" : 0,
		"gems" : 0,
		"gold" : 0
	},
	// "value" of creature, used to determine for example army strength
	"fightValue" : 0,

	// "ai value" - how valuable this creature should be for AI
	"aiValue" : 0,
	
	// normal growth in town or external dwellings
	"growth" : 0,
	
	// growth bonus from horde building
	// TODO: reconsider need of this field after configurable buildings support
	"hordeGrowth" : 0,
	
	// Creature stats in battle
	"attack" : 0,
	"defense" : 0,
	"hitPoints" : 0,
	"shots" : 0,
	"speed" : 0,
	"damage" :
	{
		"min" : 0,
		"max" : 0
	},
	// spellpoints this creature has, how many times creature may cast its spells
	"spellPoints" : 0,
	// initial size of creature army on adventure map
	"advMapAmount" :
	{
		"min" : 0,
		"max" : 0
	},
	
	// Creature to which this creature can be upgraded
	// Note that only one upgrade can be available from UI
	"upgrades" :
	[
		"anotherCreature"
	],

	// Creature is 2-tiles in size on the battlefield
	"doubleWide" : false,

	// All creature abilities, using bonus format
	"abilities" :
	[
		"someName1" : Bonus Format,
		"someName2" : Bonus Format
	],

	"hasDoubleWeek": true,
	
	"graphics" :
	{
		// name of file with creature battle animation
		"animation" : "",
		// adventure map animation def
		"map" : "",
		// path to small icon for tooltips & hero exchange window
		"iconSmall" : "",
		// path to large icon, used on town screen and in hero screen
		"iconLarge" : "",
		
		// animation parameters

		// how often creature should play idle animation
		"timeBetweenFidgets" : 1.00,
		// unused H3 property
		"troopCountLocationOffset" : 0,
		"animationTime" :
		{
			// movement animation time.
			"walk" : 1.00,

			// idle animation time. For H3 creatures this value is always 10
			"idle" : 10.00,

			// ranged attack animation time. Applicable to shooting and casting animation
			// NOTE: does NOT affects melee attacks
			// This is H3 behaviour, for proper synchronization of attack/defense animations
			"attack" : 1.00,

			// How far flying creature should move during one "round" of movement animation
			// This is multiplier to base value (200 pixels)
			"flight" : 1.00
		},
		"missile" :
		{
			// name of file for missile
			"animation" : "", 

			// (VCMI 1.1 or later only) indicates that creature uses ray animation for ranged attacks instead of missile image (e.g. Evil Eye)
			"ray" : 
			[
				{ // definition of first (top-most) line in the ray
					"start" : [ 160, 192, 0, 255 ], // color (RGBA components) of ray at starting point
					"end" : [ 160, 192, 0,  64 ]  // color (RGBA components) of ray at finishing point
				},
				{}, // definition of second from top line in the ray, identical format
				... // definitions of remaining lines, till desired width of the ray
			],
			// Frame at which shooter shoots his projectile (e.g. releases arrow)
			"attackClimaxFrame" : 0,

			// offsets between position of shooter and position where projectile should appear
			"offset" :
			{
				"upperX" : 0,
				"upperY" : 0,
				"middleX" : 0,
				"middleY" : 0,
				"lowerX" : 0,
				"lowerY" : 0
			},
			// angles from which frames in .def file were rendered, -90...90 range
			// Example below will work for file that contains following frames:
			// 1) facing top, 2) facing top-right, 3)facing right,
			// 4) facing bottom-right 5) facing bottom.
			"frameAngles" : [ -90, -45, 0, 45, 90]
		}
	},

	// names of sound files
	"sound" : 
	{
		// Creature attack enemy in melee (counter-)attack
		"attack": "",
		// Creature in "defend mode" is attacked
		"defend": "",
		// Creature killed
		"killed": "",
		// Plays in loop during creature movement
		"move": "",
		// Shooters only, creature shoots
		"shoot" : "",
		// Creature not in "defend mode" is under attack 
		"wince": "",
		
		// Creature start/end movement or teleports
		"startMoving" : "",
		"endMoving" : ""
	}
}
```