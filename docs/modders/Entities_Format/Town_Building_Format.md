# Town Building Format

# Required data

Each building requires following assets:

-   Town animation file (1 animation file)
-   Selection highlight (1 image)
-   Selection area (1 image)
-   Town hall icon (1 image)

## Town Building node

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
	"cost" : { 
		"wood" : 20,
		"ore" : 20,
		"gold" : 10000
	}, 
	
	// Allows to define additional functionality of this building, usually using logic of one of original H3 town building
	// Generally only needs to be specified for "special" buildings
	// See 'List of unique town buildings' section below for detailed description of this field
	"type" : "",
	
	// If set, building will have Lookout Tower logic - extend sight radius of a town.
	// Possible values: 
	// low - increases town sight radius by 5 tiles
	// average - sight radius extended by 15 tiles
	// high - sight radius extended by 20 tiles
	// skyship - entire map will be revealed
	// If not set, building will not affect sight radius of a town
	"height" : "average"
	
	// Resources produced each day by this building
	"produce" : { 
		"sulfur" : 1,
		"gold" : 2000
	}, 

	//determine how this building can be built. Possible values are:
	// normal  - default value. Fulfill requirements, use resources, spend one day
	// auto    - building appears when all requirements are built
	// special - building can not be built manually
	// grail   - building requires grail to be built
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
### List of unique town buildings

Following Heroes III buildings can be used as unique buildings for a town. Their functionality should be identical to a corresponding H3 building:
- `mysticPond`
- `artifactMerchant`
- `freelancersGuild`
- `magicUniversity`
- `castleGate`
- `creatureTransformer`
- `portalOfSummoning`
- `ballistaYard`
- `stables`
- `manaVortex`
- `lookoutTower`
- `library`
- `brotherhoodOfSword`
- `fountainOfFortune`
- `escapeTunnel`
- `lighthouse`
- `treasury`
- `spellPowerGarrisonBonus`
- `attackGarrisonBonus`
- `defenseGarrisonBonus`

Following HotA buildings can be used as unique building for a town. Functionality should match corresponding HotA building:
- `thievesGuild`
- `bank`

In addition to above, it is possible to use same format as [Rewardable](../Map_Objects/Rewardable.md) map objects for town buildings. In order to do that, town building type must be set to `configurable` and configuration of a rewardable object must be placed into `configuration` node 

Example 1 - Order of Fire from Inferno:
```jsonc
"special4": { // 
	"type" : "configurable",
	"requires" : [ "mageGuild1" ],
	"configuration" : {
		"visitMode" : "hero",
		"rewards" : [
			{
				"message" : "@core.genrltxt.582", // NOTE: this forces vcmi to load string from H3 text file. In order to define own string simply write your own message without '@' symbol
				"primary" : { "spellpower" : 1 }
			}
		]
	}
}
``` 

Example 2 - Mana Vortex from Dungeon
```jsonc
"special2": {
	"type" : "configurable",
	"requires" : [ "mageGuild1" ],
	"configuration" : {
		"resetParameters" : {
			"period" : 7,
			"visitors" : true
		},
		"visitMode" : "once",
		"rewards" : [
			{
				"limiter" : {
					"noneOf" : [ { "manaPercentage" : 200 } ]
				},
				"message" : "@core.genrltxt.579",
				"manaPercentage" : 200
			}
		]
	}
}
```

### Town Structure node

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