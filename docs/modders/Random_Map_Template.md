# Random Map Template

## Template format

```json
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
	"settings" :
	{
		"heroes" :
		{
			"perPlayerOnMapCap" : 1
		}
	},
	
	/// List of spells that are banned on this map. 
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
	"bannedSpells": [
		"townPortal",
		"modID:spellFromMod"
	],
	
	// Similar to bannedSpells, list of normally banned spells that are enabled on this map
	"enabledSpells" : [ ],

	/// List of artifacts that are banned on this map. 
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
	"bannedArtifacts": [
		"armageddonsBlade",
		"modID:artifactFromMod"
	],
	
	// Similar to bannedArtifacts, list of normally banned artifacts that are enabled on this map
	"enabledArtifacts" : [ ],

	/// List of secondary skills that are banned on this map. 
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
	"bannedSkills": [
		"diplomacy",
		"modID:secondarySkillFromMod"
	],

	// Similar to bannedSkills, list of normally banned skills that are enabled on this map
	"enabledSkills" : [ ],

	/// List of heroes that are banned on this map. 
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
	"bannedHeroes": [
		"lordHaart",
		"modID:heroFromMod"
	],
	
	// Similar to bannedHeroes, list of normally banned heroes that are enabled on this map
	"enabledHeroes" : [ ],

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

```json
{
	// Type of this zone. Possible values are:
	// "playerStart" - Starting zone for a "human or CPU" players
	// "cpuStart" - Starting zone for "CPU only" players
	// "treasure" - Generic neutral zone
	// "junction" - Neutral zone with narrow passages only. The rest of area is filled with obstacles.
	// "sealed" - Decorative impassable zone completely filled with obstacles
	"type" : "playerStart", 

	// relative size of zone
	"size" : 2, 
	
	// index of player that owns this zone
	"owner" : 1,
	
	// Force zone placement on specific level. Possible values:
	// "automatic" (default) - Zone level is determined automatically based on faction terrain preferences
	// "surface" - Force zone to be placed on surface level (level 0)
	// "underground" - Force zone to be placed on underground level (level 1)
	"forcedLevel" : "automatic", 
	
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
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
	"terrainTypes" : [ "sand" ], 

	// List of type hints for every town, in the order of placement. First present hint if used for each town
	"townHints" : [
		{
			// Main town has same type as main town type in zone 1
			"likeZone" : 1,
		},
		{
			// 2nd town matches terrain type of zone 4
			"relatedToZoneTerrain" : 4
		},
		{
			// 3rd town type cannot match any of the following zones. Can be integer or vector of integers
			"notLikeZone" : [1, 2, 3, 4]
		}
	],
	
	//optional, list of explicitly banned terrain types
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded
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
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded

	"allowedMonsters" : ["inferno", "necropolis"] 
	
	// These monsers will never appear in the zone
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded

	"bannedMonsters" : ["fortress", "stronghold", "conflux"]
	
	// towns allowed on this terrain
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded

	"allowedTowns" : ["castle", "tower", "rampart"] 
	
	// towns will never spawn on this terrain
	/// Identifier without modID specifier MUST exist in base game or in one of dependencies
	/// Identifier with explicit modID specifier will be silently skipped if corresponding mod is not loaded

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
		"bannedObjects" : {
			// ban a specific object from base game or from any dependent mod. Object with such ID must exist
			"randomArtifactRelic" : true,
			
			// ban object named townGate from mod 'hota.mapobjects'. Mod can be used without explicit dependency
			// If mod with such name is not loaded, this entry will be ignored and will have no effect
			"hota.mapobjects:townGate" : true,
			
			// ban only land version of Cartographer. Other versions, such as water and subterra cartographers may still appear on map
			"cartographer" : {
				"cartographerLand" : true
			}
		},

		// Configure individual common objects. 
		// Any object in this list will be excluded from regular placement rules, similarly to bannedObjects
		"commonObjects":
		[
			{
				// configure water cartographer properties
				"type" : "cartographer",
				"subtype" : "cartographerWater",
				"rmg" : {
					"value"		: 9000,
					"rarity"	: 500,
					"zoneLimit" : 2
				}
			},
			{
				// configure scholar properties. Behavior unspecified if there are multiple 'scholar' objects
				"type" : "scholar",
				"rmg" : {
					"value"		: 900,
					"rarity"	: 50,
					"zoneLimit" : 2
				}
			},
			{
				// configure town gate (hota) properties. This entry will have no effect if mod is not enabled
				"type" : "hota.mapobjects:townGate",
				"rmg" : {
					"value"		: 2000,
					"rarity"	: 100,
					"zoneLimit" : 2
				}
			}
		]
	}
}
```
