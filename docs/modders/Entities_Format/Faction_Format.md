< [Documentation](../../Readme.md) / [Modding](../Readme.md) / Entities Format / Faction Format

## Required data

In order to make functional town you also need:

### Images

-   Creature backgrounds images, 120x100 and 130x100 versions (2 images)
-   Set of puzzle map pieces (48 images)
-   Background scenery (1 image)
-   Mage guild window view (1 image)
-   Town hall background (1 image)

-   Set of town icons, consists from all possible combinations of: (8
    images total)
    -   small and big icons
    -   village and fort icons
    -   built and normal icons

-   Set for castle siege screen, consists from:
    -   Background (1 image)
    -   Destructible towers (3 parts, 3 images each)
    -   Destructible walls (4 parts, 3 images each)
    -   Static walls (3 images)
    -   Town gates (5 images)
    -   Moat (2 images)

### Animation

-   Adventure map images for village, town and capitol (3 def files)

### Music

-   Town theme music track (1 music file)

### Buildings

Each town requires a set of buildings (Around 30-45 buildings)

-   Town animation file (1 animation file)
-   Selection highlight (1 image)
-   Selection area (1 image)
-   Town hall icon (1 image)

## Faction node (root entry for town configuration)

```jsonc
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

	// Faction alignment. Can be good, neutral (default) or evil.
	"alignment" : "",

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

```jsonc
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
	// Path to town music theme, e.g. "music/castleTheme"
	"musicTheme" : "",

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

	// Small scenery for window in mage guild screen
	"guildWindow": "",

	// Background image for window in mage guild screen
	"guildBackground" : "",

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
	"tavern" :
	{
		"knight" : 5,
		"druid"  : 6
	},

	// Chance of specific spell to appear in mages guild of this town
	// If spell is missing or set to 0 it will not appear unless set as "always present" in editor
	// Spells from unavailable levels are not required to be in this list
	"guildSpells" :
	{
		"magicArrow" : 30,
		"bless"  : 10
	},

	// Which tiers in this town have creature hordes. Set to -1 to disable horde(s)
	"horde" : [ 2, -1 ], 

	// Resource given by starting bonus. If not set silo will produce wood + ore
	"primaryResource" : "gems", 

	// maximum level of mage guild
	"mageGuild" : 4,

	// war machine produced in town
	"warMachine" : "ballista"
	
	// Identifier of spell that will create effects for town moat during siege
	"moatAbility" : "castleMoat"
}
```

## Siege node

```jsonc
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

```jsonc
{
	// Numeric identifier of this building
	"id" : 0,
	
	// Localizable name of this building
	"name" : "",
	
	// Localizable decsription of this building
	"description" : "",
	
	// Optional, indicates that this building upgrades another base building
	"upgrades" : "baseBuilding",
	
	// List of town buildings that must be built before this one. See below for full format
	"requires" : [ "allOf", [ "mageGuild1" ], [ "tavern" ] ],
	
	// Resources needed to build building
	"cost" : { ... }, 
	
	// TODO: Document me: Subtype for some special buildings
	"type" : "",
	
	// TODO: Document me: Height for lookout towers and some grails
	"height" : "average"
	
	// Resources produced each day by this building
	"produce" : { ... }, 

	//determine how this building can be built. Possible values are:
	// normal  - default value. Fulfill requirements, use resources, spend one day
	// auto    - building appears when all requirements are built
	// special - building can not be built manually
	// grail   - building reqires grail to be built
	"mode" : "auto",
	
	// Buildings which bonuses should be overridden with bonuses of the current building
	"overrides" : [ "anotherBuilding ]
	
	// Bonuses, provided by this special building on build using bonus system
	"bonuses" : BONUS_FORMAT
	
	// Bonuses, provided by this special building on hero visit and applied to the visiting hero
	"onVisitBonuses" : BONUS_FORMAT
}
```

Building requirements can be described using logical expressions:

```jsonc
"requires" :
[
    "allOf", // Normal H3 "build all" mode
    [ "mageGuild1" ],
    [
        "noneOf",  // available only when none of these building are built
        [ "dwelling5A" ],
        [ "dwelling5AUpgrade" ]
    ],
    [
        "anyOf", // any non-zero number of these buildings must be built
        [ "tavern" ],
        [ "blacksmith" ]
    ]
]
```

## Structure node

```jsonc
{
	// Main animation file for this building
	"animation" : "", 
	
	// Horizontal position on town screen
	"x" : 0,
	
	// Vertical  position on town screen
	"y" : 0,
	
	// used for blit order. Higher value places structure close to screen and drawn on top of buildings with lower values
	"z" : 0, 
	
	// Path to image with golden border around building, displayed when building is selected
	"border" : "", 
	
	// Path to image with area that indicate when building is selected
	"area" : "",
	
	//TODO: describe me
	"builds": "",
	
	// If upgrade, this building will replace parent animation but will not alter its behaviour
	"hidden" : false 
}
```