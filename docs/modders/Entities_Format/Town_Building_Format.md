# Town Building Format

## Required data

Each building requires following assets:

- Town animation file (1 animation file)
- Selection highlight (1 image)
- Selection area (1 image)
- Town hall icon (1 image)

## Examples

These are just a couple of examples of what can be done in VCMI. See vcmi configuration files to check how buildings from Heroes III are implemented or other mods for more examples

####

##### Order of Fire from Inferno

```json
"special4": {
	"requires" : [ "mageGuild1" ],
	"name" : "Order of Fire",
	"description" : "Increases spellpower of visiting hero",
	"cost" : {
		"mercury" : 5,
		"gold" : 1000
	},
	"configuration" : {
		"visitMode" : "hero",
		"rewards" : [
			{
				// NOTE: this forces vcmi to load string from H3 text file. In order to define own string simply write your own message without '@' symbol
				"message" : "@core.genrltxt.582", 
				"primary" : { "spellpower" : 1 }
			}
		]
	}
}
```

##### Mana Vortex from Dungeon

```json
"special2": {
	"requires" : [ "mageGuild1" ],
	"name" : "Mana Vortex",
	"description" : "Doubles mana points of the first visiting hero each week",
	"cost" : {
		"gold" : 5000
	},
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
				"message" : "As you near the mana vortex your body is filled with new energy. You have doubled your normal spell points.",
				"manaPercentage" : 200
			}
		]
	}
}
```

#### Resource Silo with custom production

```json
"resourceSilo": {
	"name" : "Wood Resource Silo",
	"description" : "Produces 2 wood every day",
	"cost" : {
		"wood" : 10,
		"gold" : 5000
	},
	"produce" : {
		"wood": 2
	}
},
```

#### Brotherhood of Sword - bonuses in siege

```json
"special3": {
	// replaces +1 Morale bonus from Tavern
	"upgradeReplacesBonuses" : true, 
	// Gives +2 bonus to morale to town (effective only during siege)
	"bonuses": [
		{
			"type": "MORALE",
			"val": 2
		}
	],
	"upgrades" : "tavern"
},
```

#### Lighthouse - bonus to all heroes under player control

```json
"special1":       { 
	"bonuses": [
		{
			"propagator": "PLAYER_PROPAGATOR", // bonus affects everything under player control
			"type": "MOVEMENT",
			"subtype": "heroMovementSea",
			"val": 500 // +500 movement points
		}
	],
	"requires" : [ "shipyard" ]
},
```

## Town Building node

```json
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
	
	// Artifact ID of a war machine produced in this town building, if any
	"warMachine" : "ballista",
	
	// Allows to define additional functionality of this building, usually using logic of one of original H3 town building
	// Generally only needs to be specified for "special" buildings
	// See 'List of unique town buildings' section below for detailed description of this field
	"type" : "",

	// Resources produced each day by this building
	"produce" : { 
		"sulfur" : 1,
		"gold" : 2000
	},
	
	// Optional, allows this building to add fortifications during siege
	"fortifications" : {
		// Maximum health of destructible walls. Walls are only present if their health is above zero".
		// Presence of walls is required for all other fortification types
		"wallsHealth" : 3,

		// If set to true, moat will be placed in front of the walls. Requires walls presence.
		"hasMoat" : true

		// Maximum health of central tower or 0 if not present. Requires walls presence.
		"citadelHealth" : 2,
		// Maximum health of upper tower or 0 if not present. Requires walls presence.
		"upperTowerHealth" : 2,
		// Maximum health of lower tower or 0 if not present. Requires walls presence.
		"lowerTowerHealth" : 2,

		// Creature ID of shooter located in central keep (citadel). Used only if citadel is present.
		"citadelShooter" : "archer",
		// Creature ID of shooter located in upper tower. Used only if upper tower is present.
		"upperTowerShooter" : "archer",
		// Creature ID of shooter located in lower tower. Used only if lower tower is present.
		"lowerTowerShooter" : "archer",
	},

	//determine how this building can be built. Possible values are:
	// normal  - default value. Fulfill requirements, use resources, spend one day
	// auto    - building appears when all requirements are built
	// special - building can not be built manually
	// grail   - building requires grail to be built
	"mode" : "auto",
	
	// Buildings which bonuses should be overridden with bonuses of the current building
	"overrides" : [ "anotherBuilding ]
	
	// Bonuses provided by this special building if this building or any of its upgrades are constructed in town
	"bonuses" : [ BONUS_FORMAT ]
	
	// If set to true, this building will not automatically activate on new day or on entering town and needs to be activated manually on click
	// Note that such building can only be activated by visiting hero, and not by garrisoned hero.
	"manualHeroVisit" : false,
	
	// Bonuses provided by this special building if this building or any of its upgrades are constructed in town
	"bonuses" : [ BONUS_FORMAT ]
	
	
	// If the building is a market, it requires market mode.
	"marketModes" : [ "resource-resource", "resource-player" ],
	
	// Required if building offers resource-skill trade.
	// NOTE: multiple resource-skill buildings in the same town are not supported
	"marketOffer" : [ "fireMagic", "airMagic", "waterMagic", "earthMagic" ],
}
```

Building requirements can be described using logical expressions:

```json
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

#### Buildings from Heroes III

Following Heroes III buildings can be used as unique buildings for a town. Their functionality should be identical to a corresponding H3 building. H3 buildings that are not present in this list contain no hardcoded functionality. See vcmi json configuration to see how such buildings can be implemented in a mod.

- `mysticPond`
- `castleGate`
- `portalOfSummoning`
- `library`
- `escapeTunnel`
- `treasury`
- `auroraBorealis`
- `deityOfFire`

#### Buildings from other Heroes III mods

Following HotA buildings can be used as unique building for a town. Functionality should match corresponding HotA building:

- `bank`

#### Custom buildings

In addition to above, it is possible to use same format as [Rewardable](../Map_Objects/Rewardable.md) map objects for town buildings. In order to do that, configuration of a rewardable object must be placed into `configuration` json node in building config.

### Town Structure node

```json
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

#### Markets in towns

Market buildings require list of available [modes](../Map_Objects/Market.md)

##### Marketplace

```json
	"marketplace":    { "marketModes" : ["resource-resource", "resource-player"] },
```

##### Artifact merchant

```json
	"special1":       { "type" : "artifactMerchant", "requires" : [ "marketplace" ], "marketModes" : ["resource-artifact", "artifact-resource"] },
```
