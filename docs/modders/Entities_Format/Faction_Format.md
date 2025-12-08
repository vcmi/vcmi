# Faction Format

This page tells you what you need to do to make your faction work. For help, tips and advices, read the [faction help](Faction_Help.md).

## Required data

In order to make functional town, you also need:

### Images

- Creature backgrounds images, 120x100 and 130x100 versions (2 images)
- Set of puzzle map pieces (48 images)
- Background scenery (1 image)
- Mage guild window view (1 image)
- Town hall background (1 image)

- Set of town icons, consists from all possible combinations of: (8
    images total)
  - small and big icons
  - village and fort icons
  - built and normal icons

- Set for castle siege screen, consists from:
  - Background (1 image)
  - Destructible towers (3 parts, 3 images each)
  - Destructible walls (4 parts, 3 images each)
  - Static walls (3 images)
  - Town gates (5 images)
  - Moat (2 images)

### Animation

- Adventure map images for village, town and capitol (3 def files)

### Music

- Town theme music track (at least 1 music file)

### Buildings

Each town requires a set of buildings (Around 30-45 buildings)

- Town animation file (1 animation file)
- Selection highlight (1 image)
- Selection area (1 image)
- Town hall icon (1 image)

## Faction node (root entry for town configuration)

```json
// Unique faction identifier.
"myFaction" :
{
	// Main part of town description, see below
	// Optional but it should be present for playable faction
	"town" : { ... },

	// Native terrain for creatures. Creatures fighting on native terrain receive several bonuses
	"nativeTerrain" : "grass",

	// Localizable faction name, e.g. "Rampart"
	"name" : "", 

	// Description of town (e.g. history or story about town)
	"description" : "",

	// Faction alignment. Can be good, neutral (default) or evil.
	"alignment" : "",
	
	// If set to true, RMG will prefer placing towns of this faction on subterranean level of the map
	"preferUndergroundPlacement" : true,

	// Backgrounds for creature screen, two versions: 120px-height and 130-px height
	"creatureBackground"
	{
		// Paths to background images
		"120px" : "",
		"130px" : ""
	},
	
	// Identifier of boat type that is produced by shipyard and used by heroes in water taverns or prisons
	"boat" : "boatFortress",
	
	// Random map generator places player/cpu-owned towns underground if true is specified and on the ground otherwise. Parameter is unused for maps without underground.
	"preferUndergroundPlacement" : false
	
	// Optional, if set to true then faction cannot be selected on game start and will not be used for "random town" object
	"special" : false

	// Town puzzle map
	"puzzleMap" :
	{
		// Prefix for image names, e.g. "PUZCAS" for name "PUZCAS12.png"
		"prefix" : "", 
		// List of map pieces. First image will have name <prefix>00, second - <prefix>01 and so on
		"pieces" :
		[
			{
				// Position of image on screen
				"x" : 0
				"y" : 0

				//indicates order in which this image will be opened
				"index" : 0 
			},
			...
		]
	]
}
```

## Town node

```json
{
	// Field that describes behavior of map object part of town. Town-specific part of object format
	"mapObject" : 
	{
		// Optional, controls what template will be used to display this object.
		// Whenever player builds a building in town game will test all applicable templates using
		// tests with matching name and on success - set such template as active.
		// There are 3 predefined filters: "village", "fort" and "capitol" that emulate H3 behavior
		"filter" : {
			"capitol" : [ "anyOf", [ "capitol" ], [ "castle" ] ]
		},

		// List of templates that represent this object. For towns only animation is required
		// See object template description for other fields that can be used.
		"templates" : {
			"village" : { "animation" : "" },
			"castle"  : { "animation" : "" },
			"capitol" : { "animation" : "" }
		}
	},

	//icons, small and big. Built versions indicate constructed during this turn building.
	"icons" : 
	{
		"village" : {
			"normal" : {
				"small" : "modname/icons/hall-small.bmp",
				"large" : "modname/icons/hall-big.bmp"
			},
			"built" : {
				"small" : "modname/icons/hall-builded-small.bmp",
				"large" : "modname/icons/hall-builded-big.bmp"
			}
		},
		"fort" : {
			"normal" : {
				"small" : "modname/icons/fort-small.bmp",
				"large" : "modname/icons/fort-big.bmp"
			},
			"built" : {
				"small" : "modname/icons/fort-builded-small.bmp",
				"large" : "modname/icons/fort-builded-big.bmp"
			}
		}
	},
	// List of town music themes, e.g. [ "music/castleTheme" ]
	// At least one music file is required
	"musicTheme" : [ "" ],

	// List of structures which represents visible graphical objects on town screen.
	// See detailed description below
	"structures" : 
	{
		"building1" : { ... },
		         ...
		"building9" : { ... } 
	},

	// List of names for towns on adventure map e.g. "Dunwall", "Whitestone"
	// Does not have any size limitations
	"names" : [ "", ""],

	// Background scenery for town screen, size must be 800x374
	"townBackground": "",

	// Small scenery for window in mage guild screen; each element of array is for seperate mage guild level image (if only one element, then this will always used)
	"guildWindow": [""],

	// Background image for window in mage guild screen; each element of array is for seperate mage guild level image (if only one element, then this will always used)
	"guildBackground" : [""],

	// Video for tavern window
	"tavernVideo" : "",
	
	// Path to building icons for town hall
	"buildingsIcons": "HALLCSTL.DEF", 

	// Background image for town hall window
	"hallBackground": "",

	// List of buildings available in each slot of town hall window
	// Note that size of gui is limited to 5 rows and 4 columns
	"hallSlots": 
	[
		[ [ "buildingID1" ], [ "buildingID2", "buildingID3" ] ],
		   ...
	],
	// List of creatures available on each tier. Number of creatures on each tier
	// is not hardcoded but it should match with number of dwelling for each level.
	// For example structure below would need buildings with these id's:
	// first tier: 30 and 37, second tier: 31, third tier: 32, 39, 46
	"creatures" :
	[
		["centaur", "captainCentaur"],
		["dwarf"],
		["elf", "grandElf", "sharpshooter"],
		...
	 ],

	// Buildings, objects in town that affect mechanics. See detailed description below
	"buildings" : 
	{
		"building1" : { ... },
		         ...
		"building9" : { ... } 
	},
	// Description of siege screen, see below
	"siege" : { ... },

	// Chance for a hero class to appear in this town, creates pair with same field in class format 
	// Used for situations where chance was not set in "tavern" field, chance will be determined as:
	// square root( town tavern chance * hero class tavern chance )
	"defaultTavern" : 5,

	// Chance of specific hero class to appear in this town
	// Mirrored version of field "tavern" from hero class format
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
	"tavern" :
	{
		"knight" : 5,
		"druid"  : 6,
		"modID:classFromMod" : 4
	},

	// Chance of specific spell to appear in mages guild of this town
	// If spell is missing or set to 0 it will not appear unless set as "always present" in editor
	// Spells from unavailable levels are not required to be in this list
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded

	"guildSpells" :
	{
		"magicArrow" : 30,
		"bless"  : 10,
		"modID:spellFromMod" : 20
	},

	// Which tiers in this town have creature hordes. Set to -1 to disable horde(s)
	"horde" : [ 2, -1 ], 

	// Resource given by starting bonus. If not set silo will produce wood + ore
	"primaryResource" : "gems", 

	// maximum level of mage guild
	"mageGuild" : 5,

	// Coordinates of spell images in mage guild screen. Should contain the maximum level of mage guild amount of elements. Example are HoMM3 defaults:
	"guildSpellPositions" : [
		[ { "x": 222, "y": 445 }, { "x": 312, "y": 445 }, { "x": 402, "y": 445 }, { "x": 520, "y": 445 }, { "x": 610, "y": 445 }, { "x": 700, "y": 445 } ],
		[ { "x":  48, "y":  53 }, { "x":  48, "y": 147 }, { "x":  48, "y": 241 }, { "x":  48, "y": 335 }, { "x":  48, "y": 429 } ],
		[ { "x": 570, "y":  82 }, { "x": 672, "y":  82 }, { "x": 570, "y": 157 }, { "x": 672, "y": 157 } ],
		[ { "x": 183, "y":  42 }, { "x": 183, "y": 148 }, { "x": 183, "y": 253 } ],
		[ { "x": 491, "y": 325 }, { "x": 591, "y": 325 } ]
	],

	// Coordinates of window image in mage guild screen. Example is HoMM3 default:
	"guildWindowPosition": { "x": 332, "y": 76 },

	// Identifier of spell that will create effects for town moat during siege
	"moatAbility" : "castleMoat"
}
```

## Siege node

```json
// Describes town siege screen
// Comments in the end of each graphic position indicate specify required suffix for image
// Note: one not included image is battlefield background with suffix "BACK"
{
	// shooter creature name
	"shooter" : "archer",

	// Large icon of towers, for use in battle queue
	"towerIconLarge" : "",

	// (Small icon of towers, for use in battle queue
	"towerIconSmall" : "",

	// Prefix for all siege images. Final name will be composed as <prefix><suffix>
	"imagePrefix" : "SGCS",

	// Descriptions for towers. Each tower consist from 3 parts:
	// tower itself - two images with untouched and destroyed towers
	// battlement or creature cover - section displayed on top of creature
	// creature using type from "shooter" field above
	"towers":
	{
		// Top tower description
		"top" :
		{
			"tower" :      { "x": 0, "y": 0}, // "TW21" ... "TW22"
			"battlement" : { "x": 0, "y": 0}, // "TW2C"
			"creature" :   { "x": 0, "y": 0}
		},
		// Central keep description
		"keep" :
		{
			"tower" :      { "x": 0, "y": 0}, // "MAN1" ... "MAN2"
			"battlement" : { "x": 0, "y": 0}, // "MANC"
			"creature" :   { "x": 0, "y": 0}
		},
		// Bottom tower description
		"bottom" :
		{
			"tower" :      { "x": 0, "y": 0}, // "TW11" ... "TW12"
			"battlement" : { "x": 0, "y": 0}, // "TW1C"
			"creature" :   { "x": 0, "y": 0}
		},
	},
	// Two parts of gate: gate itself and arch above it
	"gate" :
	{
		"gate" : { "x": 0, "y": 0}, // "DRW1" ... "DRW3" and "DRWC" (rope)
		"arch" : { "x": 0, "y": 0}  // "ARCH"
	},
	// Destructible walls. In this example they are ordered from top to bottom
	// Each of them consist from 3 files: undestroyed, damaged, destroyed
	"walls" : 
	{
		"upper"     : { "x": 0, "y": 0}, // "WA61" ... "WA63"
		"upperMid"  : { "x": 0, "y": 0}, // "WA41" ... "WA43"
		"bottomMid" : { "x": 0, "y": 0}, // "WA31" ... "WA33"
		"bottom"    : { "x": 0, "y": 0}  // "WA11" ... "WA13"
	},

	// Two pieces for moat: moat itself and moat bank
	"moat" :
	{
		"bank" : { "x" : 0, "y" : 0 }, // "MOAT"
		"moat" : { "x" : 0, "y" : 0 }  // "MLIP"
	},

	// Static non-destructible walls. All of them have only one piece
	"static" : 
	{
		// Section between two bottom destructible walls
		"bottom" : { "x": 0, "y": 0}, // "WA2"

		// Section between two top destructible walls
		"top" : { "x": 0, "y": 0}, // "WA5"

		// Topmost wall located behind hero
		"background" : { "x": 0, "y": 0} // "TPWL"
	}
}
```

## Building node

See [Town Building Format](Town_Building_Format.md)

## Structure node

See [Town Building Format](Town_Building_Format.md)
