Ideally, template format should be 100% compatible with OH3 format and
bring additional improvements.

## List of currently available templates

-   Analogy
-   Upgrade
-   Golden Ring
-   Unfair Game
-   Jebus Cross

## Template format

``` javascript
/// Unique template name
"Triangle" : 
{
	//optional name - useful to have several template variations with same name (since 0.99)
	"name" : "Custom template name",
	"description" : "Brief description of template, recommended setting or rules".

	/// Minimal and maximal size of the map. Possible formats:
	/// Size code: s, m, l or xl for size with optional suffix "+u" for underground
	/// Numeric size, e.g.  120x120x1 (width x height x depth). Note that right now depth can only be 0 or 1
	"minSize" : "m",
	"maxSize" : "xl+u",

	/// Number of players that will be present on map (human or AI)
	"players" : "2-4",

	/// Number of AI-only players
	"cpu" : "2",

	///Optional parameter allowing to prohibit some water modes. All modes are allowed if parameter is not specified
	"allowedWaterContent" : ["none", "normal", "islands"]

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
        //"type" can be "guarded" (default), "wide", "fictive" or "repulsive"
        //"wide" connections have no border, or guard. "fictive" and "repulsive" connections are virtual -
        //they do not create actual path, but only attract or repulse zones, respectively
	]
}
```

## Zone format

``` javascript
{
	"type" : "playerStart", //"cpuStart" "treasure" "junction"
	"size" : 2, //relative size of zone
	"owner" : 1, //player owned this zone
	"playerTowns" : {
		"castles" : 1
		//"towns" : 1
	},
	"neutralTowns" : {
		//"castles" : 1
		"towns" : 1
	},
	"townsAreSameType" : true,
	"monsters" : "normal", //"weak" "strong", "none" - All treasures will be unguarded

	"terrainTypes" : [ "sand" ], //possible terrain types. All terrains will be available if not specified
	"bannedTerrains" : ["lava", "asphalt"] //optional

	"matchTerrainToTown" : false, //if true, terrain for this zone will match native terrain of player faction
	"minesLikeZone" : 1,
	"treasureLikeZone" : 1
	"terrainTypeLikeZone" : 3

	"allowedMonsters" : ["inferno", "necropolis"] //factions of monsters allowed on this zone
	"bannedMonsters" : ["fortress", "stronghold", "conflux"] //These monsers will never appear in the zone
	"allowedTowns" : ["castle", "tower", "rampart"] //towns allowed on this terrain
	"bannedTowns" : ["necropolis"] //towns will never spawn on this terrain

	"mines" : {
		"wood" : 1,
		"ore" : 1,
	},

	"treasure" : [
		{
			"min" : 2100,
			"max": 3000,
			"density" : 5
		}
		  ...
	]
}
```