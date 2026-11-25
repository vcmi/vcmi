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
	"allowedWaterContent" : ["none", "normal", "islands"],

	/// Multiplier for Penrose tile size used to assign map tiles to zones. Default 1.0, allowed range 0.25-2.0.
	/// Smaller values give smoother zone edges and cost more generation time.
	"penroseTileScale" : 1.0
	
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

	/// Objects banned, customized, required or added in every zone, see "Custom objects format" below
	"customObjects" : { },

	/// Joining settings for guards placed inside every zone. Zone monsterJoining overrides this block per field.
	/// See "Monster joining" below. Connection guards are never affected.
	"monsterJoining" : {
		"joiningPercentage" : 50,
		"joinOnlyForMoney" : true
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
		//"road": "true" always builds a road; "false" never does; "random" (default) picks
		//roads that form one tree joining every zone with a town and drops dead ends into
		//town-less zones. "fictive" and "repulsive" connections never get a road.
		//"wide" connections never get a random road.
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

	// Joining settings for guards placed inside this zone. Overrides the template block per field.
	"monsterJoining" : {
		"character" : "hostile",
		"joiningPercentage" : 50,
		"joinOnlyForMoney" : true
	}, 

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

	// Custom objects will have same configuration as in linked zone.
	// The linked zone's effective config (after merging the template block) is copied; this zone's own customObjects block is ignored.
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

	// Objects banned, customized, required or added in this zone, see "Custom objects format" below
	"customObjects" : { }
}
```

## Monster joining

The `monsterJoining` block can be set at the template root and in a zone. Per field, a zone value overrides the template value. A field set in neither place keeps the generator default. Connection guards (border guards, subterranean-gate guards, two-way monolith guards) are never affected: they stay Hostile, use the global joining percentage, and may join for free.

Key names match the monster object in the VCMI map format. HoTA's template editor labels map as Disposition → `character`, Custom: N → `custom` + `aggression` N, Joining Percent → `joiningPercentage`, Join only for money → `joinOnlyForMoney`.

- `character`: `compliant` | `friendly` | `aggressive` | `hostile` | `savage` | `custom`. Default `hostile`.
- `aggression`: integer 1..10, used only with `custom`. `custom` without `aggression` logs an error and falls back to `hostile`.
- `joiningPercentage`: integer 0..100. 0 = never joins. Unset keeps -1 on the object, so the global `creatures.joiningPercentage` (100) applies.
- `joinOnlyForMoney`: boolean. Default `false`.

```json
{
	"character" : "custom",
	"aggression" : 7,
	"joiningPercentage" : 50,
	"joinOnlyForMoney" : true
}
```

HotA-parity at template level:

```json
"monsterJoining" : {
	"joiningPercentage" : 50,
	"joinOnlyForMoney" : true
}
```

## Custom objects format

The `customObjects` block can be set at the template root and in a zone. The template block is merged into every zone first:

- `bannedCategories` are combined.
- `bannedObjects`, `commonObjects` and `requiredObjects` entries replace the template entry with the same id. A zone `bannedObjects` value of `false` lifts a template ban, including a category ban, for that object. A `requiredObjects` count of 0 removes an inherited entry.
- `pandoraBoxes` are concatenated: template entries first, then zone entries.
- `seerHuts` are concatenated: template entries first, then zone entries.
- `spellScrolls` replace the template entry with the same spell; bans never remove `spellScrolls`, `seerHuts` or required scrolls.

```json
{
	// All of objects of this kind will be removed from zone
	// Possible values: "all", "none", "creatureBank", "bonus", "dwelling", "resource", "resourceGenerator", "spellScroll", "randomArtifact", "pandorasBox", "questArtifact", "seerHut", "other"
	"bannedCategories" : ["all", "dwelling", "creatureBank", "other"],

	// Specify object types and subtypes. true bans the object, false allows it even when its category is banned.
	"bannedObjects" : {
		// ban a specific object from base game or from any dependent mod. Object with such ID must exist
		"randomArtifactRelic" : true,
		
		// ban object named townGate from mod 'hota.mapobjects'. Mod can be used without explicit dependency
		// If mod with such name is not loaded, this entry will be ignored and will have no effect
		"hota.mapobjects:townGate" : true,
		
		// ban only land version of Cartographer. Other versions, such as water and subterra cartographers may still appear on map
		"cartographer" : {
			"cartographerLand" : true
		},

		// allow this object in this zone even if the template banned its category
		"dragonUtopia" : false
	},

	// Configure individual common objects.
	// The placer skips the default pool entry with the same id and uses this configuration instead.
	// A template-level entry applies to every zone that does not override that id.
	// A missing or 0 value or rarity takes the object's own rmg value from its object config.
	// An entry for an object without one is ignored with an error. A missing or 0 zoneLimit
	// means no limit per zone, and the object's map-wide limit (mapLimit) no longer applies.
	// To remove an object, use bannedObjects.
	// A randomDwellingLvl entry becomes a dwelling of the zone's faction and replaces the native dwelling of that level,
	// other native dwellings keep spawning as usual. Without rmg.value it is valued like that native dwelling
	// (creature AI value x growth, increased by the number of zones of that faction), as in SoD.
	// Without rmg.rarity it is as common as native dwellings (40)
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
			// scholar rarity only; value comes from the object's rmg config
			"type" : "scholar",
			"rmg" : {
				"rarity"	: 50
			}
		},
		{
			// scholar value only; rarity comes from the object's rmg config
			"type" : "scholar",
			"rmg" : {
				"value"		: 900
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
		},
		{
			// level 7 dwelling of the zone's faction with custom value, at most 2 per zone
			"type" : "randomDwellingLvl",
			"subtype" : "objectLvl7",
			"rmg" : {
				"value"		: 15000,
				"zoneLimit" : 2
			}
		}
	],

	// Spell scrolls added to the zone treasure pool with a fixed spell, on top of the built-in scrolls (see below)
	"spellScrolls" :
	[
		{ "spell" : "townPortal", "rmg" : { "rarity" : 9999, "zoneLimit" : 1 } },
		{ "spell" : "fly", "rmg" : { "value" : 6000 } },
		{ "spell" : "magicArrow" }
	],

	// Required objects that will be added to this zone guarded according to rmg.value of the individual objects
	// They are always generated, each one on its own, never as a part of a larger treasure pile
	// count 0 removes a required object inherited from the template
	"requiredObjects" : {
		"refugeeCamp" : 5,
		// If there are several versions (e.g. standard and cannonYard) a random selection is chosen
		"warMachineFactory" : 5,
		// Versions can be specified with optional guard information
		"creatureGeneratorCommon" : { "unicornGlade" : { "count" : 3, "guard" : 10000 } },
		// Random dwellings become a dwelling of the zone's faction (randomDwellingLvl: of the given level)
		// Without "guard", a randomDwellingLvl entry is guarded according to the value of that native dwelling (see commonObjects)
		"randomDwellingLvl" : { "objectLvl7" : { "count" : 1, "guard" : 30000 } },
		// Artifacts can be specified
		"artifact" : { "headOfLegion" : 1, "armsOfLegion" : 1, "torsoOfLegion" : 1, "loinsOfLegion" : 1, "legsOfLegion" : 1 },
		// Spell scrolls: keys are spells; without "guard" the guard follows the scroll's default value
		"spellScroll" : { "dimensionDoor" : 2, "townPortal" : { "count" : 1, "guard" : 20000 } },
		// Mod objects can be specified
		"hota.mapobjects:townGate" : { "count" : 1, "guard" : 100000 }
	},

	// Pandora boxes added to the zone treasure pool, on top of the default ones (see below)
	"pandoraBoxes" :
	[
		// HoTA "Use default" equivalent: the value defaults to the default 5000 experience box
		{ "experience" : 5000, "rmg" : { "rarity" : 9999, "zoneLimit" : 1 } },

		// value and rarity default to the default 10000 gold box
		{ "resources" : { "gold" : 10000 } },

		// one predefined spell
		{ "spells" : [ "townPortal" ], "rmg" : { "value" : 8000, "rarity" : 2 } },

		// several predefined spells; only gold, experience and nativeCreatures boxes have default rmg values, so value and rarity are required
		{ "spells" : [ "fly", "dimensionDoor", "townPortal" ], "rmg" : { "value" : 20000, "rarity" : 2 } },

		// creatures of the zone's faction, as in the default creature boxes: one box per level 1 creature (basic and upgraded)
		{ "rmg" : { "nativeCreatures" : { "level" : 1 }, "zoneLimit" : 2 } },

		// one box for the upgraded level 7 creature of the zone's faction
		{ "rmg" : { "nativeCreatures" : { "level" : 7, "kind" : "upgrade" }, "zoneLimit" : 1 } },

		// creatures, only in Castle and neutral zones
		{ "creatures" : [ { "type" : "angel", "amount" : 5 } ], "rmg" : { "value" : 25000, "rarity" : 3, "zoneFactions" : [ "castle", "neutral" ] } },

		// gold and an artifact
		{ "resources" : { "gold" : 5000 }, "artifacts" : [ "centaurAxe" ], "rmg" : { "value" : 9000, "rarity" : 5 } }
	],

	// Seer huts filling quest-artifact slots before the built-in huts (see below)
	"seerHuts" :
	[
		// 7000 experience; rarity defaults to the built-in experience hut
		{ "experience" : 7000, "rmg" : { "rarity" : 9999, "zoneLimit" : 1 } },

		// 10000 gold; value and rarity default to the built-in gold hut
		{ "resources" : { "gold" : 10000 } },

		// upgraded level 3 creature of the zone's faction, as the built-in creature huts
		{ "rmg" : { "nativeCreatures" : { "level" : 3, "kind" : "upgrade" }, "zoneLimit" : 1 } },

		// artifact and spell; value and rarity required; only in Rampart and neutral zones
		{ "artifacts" : [ "centaurAxe" ], "spells" : [ "fly" ], "rmg" : { "value" : 9000, "rarity" : 5, "zoneFactions" : [ "rampart", "neutral" ] } }
	]
}
```

### Pandora boxes

A `pandoraBoxes` entry is the `options` block of a Pandora box in a `.vmap` map, plus an `rmg` block. Content can use the short form (`experience`, `mana`, `morale`, `luck`, `resources`, `primarySkills`, `secondarySkills`, `artifacts`, `spells`, `creatures`) or the `rewardable` block that the map editor saves. Fields left out of a `rewardable` block are read as 0, as in a map; set `manaPercentage` and `movePercentage` of each reward to -1, or the reward sets the hero's mana and movement points to 0.

The `rmg` block:

- `value`: treasure value of the box. Gold-only and experience-only boxes default to the value of the default box with the same amount: `pandoras.valueMultiplierGold` or `pandoras.valueMultiplierExperience` in [randomMap.json](../../config/randomMap.json) per 5000. Required for other boxes.
- `rarity`: probability of the box in the treasure pool. Gold-only boxes default to 5 and experience-only boxes to 20, as the default boxes. Required for other boxes.
- `zoneLimit`: maximum number of boxes of this entry per zone. Default 0, unlimited.
- `nativeCreatures`: the box holds creatures of the zone's faction, as the default creature boxes: for each creature of that faction, basic and upgraded, a box with the default amount and value. `level` limits it to one creature level; 0 or no `level` means all levels. `kind` of `basic` keeps creatures that are not an upgrade of another creature of the faction; `upgrade` keeps every upgrade, including each branch and each step of a chain. Without `kind`, both are included. `kind` needs `level` 1 or more and is ignored with a warning otherwise. The entry has no other content. Rarity defaults to 3, as the default creature boxes. `zoneLimit` applies to each creature separately.
- `zoneFactions`: factions of the zones that can get the box. A zone's faction is the faction of its towns; a zone without towns gets a random faction or `neutral`. Default: all zones.

A box is gold-only when its only content is `resources` with only `gold`, and experience-only when its only content is `experience`. Gold-only, experience-only and `nativeCreatures` boxes have default values. An entry without a default `value` or `rarity` that does not set it is ignored with an error in the log.

Bans never remove custom boxes, including a `pandorasBox` category ban, which only removes the default boxes. Custom boxes are never placed in water zones, and a box worth more than any treasure range of the zone is never placed. The generator adds a guard to the box according to its value, as for any other treasure.

### Seer huts

A `seerHuts` entry uses the same content format and `rmg` fields as [Pandora boxes](#pandora-boxes). Differences:

- Gold-only and experience-only entries default `value` to the amount and `rarity` as the built-in gold and experience huts. `nativeCreatures` defaults amount, value and rarity as the built-in creature huts. Every other entry must set `value` and `rarity`.
- `value` is the reward value: a candidate above the zone's maximum treasure is dropped, and the quest artifact in the other zone is guarded by that value. The hut itself is always placed at the built-in seer hut pool value.
- Content must load to exactly one reward. The short form always does. A `rewardable` block with any other number of `info` entries is ignored with an error in the log.
- The mission is always a quest artifact of the zone, placed in another zone. Custom missions are not supported.
- A zone gets one hut per quest artifact it holds. Custom entries take those slots first, picked by rarity, each capped by its `zoneLimit`. Remaining slots go to built-in huts. Set `zoneLimit` to leave room for built-in huts.

Bans never remove custom huts, including a `seerHut` category ban, which only removes the built-in huts.

### Spell scrolls

A `spellScrolls` entry adds one scroll with a fixed spell to the zone treasure pool, on top of the built-in scrolls (one per spell level). The spell is spawned even if it is banned on the map (`bannedSpells`, or disabled by default).

- `spell`: spell identifier. Required. Mod scope is allowed (`"mod:spell"`). An unknown spell drops the entry.
- `rmg.value`: treasure value. Default: `scrolls.value` in [randomMap.json](../../config/randomMap.json) for the spell's level (500, 2000, 3000, 4000, 5000 for levels 1-5). An entry without a value whose level has no `scrolls.value` is ignored with an error in the log.
- `rmg.rarity`: probability of the scroll in the treasure pool. Default 30, as the built-in scrolls.
- `rmg.zoneLimit`: maximum number of scrolls of this entry per zone. Default 0, unlimited.

Bans never remove custom scrolls, including a `spellScroll` category ban, which only removes the built-in scrolls. Custom scrolls are never placed in water zones, and a scroll worth more than any treasure range of the zone is never placed. The generator adds a guard to the scroll according to its value, as for any other treasure.

`requiredObjects.spellScroll` keys are spells. Count 0 removes an inherited entry. Without `guard`, the guard follows the scroll's default value.
