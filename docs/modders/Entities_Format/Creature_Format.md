# Creature Format

This page tells you what you need to do to make your creature work. For help, tips and advices, read the [creature help](Creature_Help.md).

## Required data

In order to make functional creature you also need:

### Animation

- Battle animation (1 def file)
- Set of rendered projectiles (1 def files, shooters only)
- Adventure map animation (1 def file)

### Images

- Small portrait for hero exchange window (1 image)
- Large portrait for hero window (1 image)

### Sounds

- Set of sounds (up to 8 sounds)

## Format

```json
// camelCase unique creature identifier
"creatureName" : 
{
	// Translatable names for this creature
	"name" :
	{
		"singular" : "Creature",
		"plural" : "Creatures"
	},

	// Description of creature
	"description" : "",

	"level" : 0,

	// Marks this object as special and not available by default
	"special" : true, 
	
	// If set, this creature will never be picked as random monster on premade maps and will not appear in Refugee Camp
	// Random map generator does not checks for this flag and can still pick this creature
	"excludeFromRandomization" : false,

	// Faction this creature belongs to. Examples: castle, rampart
	"faction" : "", 

	// Cost to recruit this creature, zero values can be omitted.
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

	// Describes how valuable this creature is to AI. Usually similar to fightValue
	"aiValue" : 0,
	
	// Basic growth of this creature in town or in external dwellings
	"growth" : 0,
	
	// Bonus growth of this creature from built horde, if any
	"horde" : 0,
	
	// Creature stats in battle
	"attack" : 0,
	"defense" : 0,
	"hitPoints" : 0,
	"speed" : 0,
	"damage" :
	{
		"min" : 0,
		"max" : 0
	},

	// Number of shots this creature has, required only for ranged units
	"shots" : 0,

	// Spell points this creature has (usually equal to number of casts)
	"spellPoints" : 0,

	// Initial size of random stacks on adventure map
	"advMapAmount" :
	{
		"min" : 0,
		"max" : 0
	},
	
	// List of creatures to which this one can be upgraded
	"upgrades" :
	[
		"anotherCreature"
	],

	// If set, creature will be two tiles wide on battlefield
	"doubleWide" : false,

	// Creature abilities described using Bonus system
	"abilities" :
	[
		"someName1" : Bonus Format,
		"someName2" : Bonus Format
	],

	// creature may receive "week of" events
	"hasDoubleWeek": true,
	
	"graphics" :
	{
		// File with animation of this creature in battles
		"animation" : "",
		// File with animation of this creature on adventure map
		"map" : "",
		// Optional. Object mask that describes on which tiles object is visible/blocked/activatable
		"mapMask" : [ "VV", "VA" ],

		// Small icon for this creature, used for example in exchange screen
		"iconSmall" : "",
		// Large icon for this creature, used for example in town screen
		"iconLarge" : "",

		// Optional. Images of creatures when attacked on adventure map
		"mapAttackFromRight": "",
		"mapAttackFromLeft": "",
		
		// animation parameters

		// how often creature should play idle animation
		"timeBetweenFidgets" : 1.00,

		"animationTime" :
		{
			// movement animation time factor
			"walk" : 1.00,

			// idle animation time. For H3 creatures this value is always 10
			"idle" : 10.00,

			// ranged attack animation time. Applicable to shooting and casting animation
			// NOTE: does NOT affects melee attacks
			// This is H3 behaviour, likely for proper synchronization of attack/defense animations
			"attack" : 1.00,
		},
		"missile" :
		{
			// name of file for missile
			"animation" : "", 

			// indicates that creature uses ray animation for ranged attacks instead of missile image (e.g. Evil Eye)
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

			// Position where projectile image appears during shooting in specific direction
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
	},
	
	// Stack experience, using bonus system
	"stackExperience" : [
		{
			// Bonus description
			"bonus" : { BONUS_FORMAT },
			// Per-level value of bonus. Must have 10 elements
			"values" : [
				0, 0, 1, 1, 2, 2, 3, 3, 4, 4
			]
		},
		...
	]
}
```
