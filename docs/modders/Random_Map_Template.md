# Random Map Template

## Template format

``` javascript
/// Unique template name
"Triangle" : 
{
	//Optional name - useful to have several template variations with same name
	"name" : "Custom template name",
	//Any info you want to be displayed in random map menu
	"description" : "Detailed info and recommended rules",

	/// Minimal and maximal size of the map. Possible formats:
	/// Size code: s, m, l or xl for size with optional suffix "+u" for underground
	/// Numeric size, e.g.  120x120x1 (width x height x depth). Note that right now depth can only be 0 or 1
	"minSize" : "m",
	"maxSize" : "xl+u",

	/// Number of players that will be present on map (human or AI)
	"players" : "2-4",

	/// Since 1.4.0 - Optional, number of human-only players (as in original templates)
	"humans" : "1-4",

	///Optional parameter allowing to prohibit some water modes. All modes are allowed if parameter is not specified
	"allowedWaterContent" : ["none", "normal", "islands"]
	
	/// List of game settings that were overriden by this template. See config/gameConfig.json in vcmi install directory for possible values
	/// Settings defined here will always override any settings from vcmi or from mods
	"settings" : {
			"heroes" : {
				"perPlayerOnMapCap" : 1
			}
		}
	},

	/// List of named zones, see below for format description
	"zones" :
	{
		"zoneA" : { ... },
		"zoneB" : { ... },
		"zoneC" : { ... }
	},
	"connections" :
	[
		{ "a" : "zoneA", "b" : "zoneB", "guard" : 5000, "road" : "false" },
		{ "a" : "zoneA", "b" : "zoneC", "guard" : 5000, "road" : "random" },
		{ "a" : "zoneB", "b" : "zoneC", "type" : "wide" }
		//"type" can be "guarded" (default), "wide", "fictive", "repulsive" or "forcePortal"
		//"wide" connections have no border, or guard. "fictive" and "repulsive" connections are virtual -
		//they do not create actual path, but only attract or repulse zones, respectively
	]
}
```

## Zone format

``` javascript
{
	// Type of this zone. Possible values are:
	// "playerStart", "cpuStart", "treasure", "junction"
	"type" : "playerStart", 

	// relative size of zone
	"size" : 2, 
	
	// index of player that owns this zone
	"owner" : 1, 
	
	// castles and towns owned by player in this zone
	"playerTowns" : {
		"castles" : 1
		"towns" : 1
	},
	
	// castles and towns that are neutral on game start in this zone
	"neutralTowns" : {
		//"castles" : 1
		"towns" : 1
	},
	
	// if true, all towns generated in this zone will belong to the same faction
	"townsAreSameType" : true,
	
	//"weak" "strong", "none" - All treasures will be unguarded
	"monsters" : "normal", 

	//possible terrain types. All terrains will be available if not specified
	"terrainTypes" : [ "sand" ], 
	
	//optional, list of explicitly banned terrain types
	"bannedTerrains" : ["lava", "asphalt"] 

	// if true, terrain for this zone will match native terrain of player faction. Used only in owned zones
	"matchTerrainToTown" : false, 
	
	// Mines will have same configuration as in linked zone
	"minesLikeZone" : 1,
	
	// Treasures will have same configuration as in linked zone
	"treasureLikeZone" : 1,
	
	// Terrain type will have same configuration as in linked zone
	"terrainTypeLikeZone" : 3,

	// Custom objects will have same configuration as in linked zone
	"customObjectsLikeZone" : 1,

	// factions of monsters allowed on this zone
	"allowedMonsters" : ["inferno", "necropolis"] 
	
	// These monsers will never appear in the zone
	"bannedMonsters" : ["fortress", "stronghold", "conflux"]
	
	// towns allowed on this terrain
	"allowedTowns" : ["castle", "tower", "rampart"] 
	
	// towns will never spawn on this terrain
	"bannedTowns" : ["necropolis"] 

	// List of mines that will be added to this zone
	"mines" : {
		"wood" : 1,
		"ore" : 1,
	},

	// List of treasures that will be placed in this zone
	"treasure" : [
		{
			"min" : 2100,
			"max": 3000,
			"density" : 5
		}
		  ...
	],

	// Objects with different configuration than default / set by mods
	"customObjects" :
	{
		// All of objects of this kind will be removed from zone
		// Possible values: "all", "none", "creatureBank", "bonus", "dwelling", "resource", "resourceGenerator", "spellScroll", "randomArtifact", "pandorasBox", "questArtifact", "seerHut", "other
		"bannedCategories" : ["all", "dwelling", "creatureBank", "other"],
		// Specify object types and subtypes
		"bannedObjects" :["core:object.randomArtifactRelic"],
		// Configure individual common objects - overrides banned objects
		"commonObjects":
		[
			{
				"id" : "core:object.creatureBank.dragonFlyHive",
				"rmg" : {
					"value"		: 9000,
					"rarity"	: 500,
					"zoneLimit" : 2
				}
			}
		]
	}
}
```