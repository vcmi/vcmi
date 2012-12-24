#pragma once

/*
 * GameConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

namespace GameConstants
{
	const std::string VCMI_VERSION = "VCMI 0.9";

	/* 
	 * DATA_DIR contains the game data (Data/, MP3/, ...).
	 * BIN_DIR is where the vcmiclient/vcmiserver binaries reside
	 * LIB_DIR is where the AI libraries reside (linux only)
	 */
	#if defined(_WIN32)
		const std::string DATA_DIR = ".";
		const std::string BIN_DIR = ".";
		const std::string LIB_DIR = ".";
		const std::string SERVER_NAME = "VCMI_server.exe";
		const std::string LIB_EXT = "dll";
		const std::string PATH_SEPARATOR = "\\";
	#elif defined(__APPLE__)
		const std::string DATA_DIR = "../Data";
        const std::string BIN_DIR = ".";
        const std::string LIB_DIR = ".";
        const std::string SERVER_NAME = "./vcmiserver";
        const std::string LIB_EXT = "dylib";
        const std::string PATH_SEPARATOR = "/";
    #else
        #ifndef M_DATA_DIR
		#error M_DATA_DIR undefined.
		#else
		const std::string DATA_DIR = M_DATA_DIR;
		#endif
		#ifndef M_BIN_DIR
		#error M_BIN_DIR undefined.
		#else
		const std::string BIN_DIR = M_BIN_DIR;
		#endif
		#ifndef M_LIB_DIR
		#error M_LIB_DIR undefined.
		#else
		const std::string LIB_DIR = M_LIB_DIR;
		#endif
		const std::string SERVER_NAME = "vcmiserver";
		const std::string LIB_EXT = "so";
		const std::string PATH_SEPARATOR = "/";
	#endif

	const int BFIELD_WIDTH = 17;
	const int BFIELD_HEIGHT = 11;
	const int BFIELD_SIZE = BFIELD_WIDTH * BFIELD_HEIGHT;

	const int ARMY_SIZE = 7;

	const int CREATURES_COUNT = 197;
	const int CRE_LEVELS = 10;
	const int F_NUMBER = 9; //factions (town types) quantity
	const int PLAYER_LIMIT = 8; //player limit per map
	const int MAX_HEROES_PER_PLAYER = 8;
	const int ALL_PLAYERS = 255; //bitfield
	const int HEROES_PER_TYPE=8; //amount of heroes of each type
	const int SKILL_QUANTITY=28;
	const int SKILL_PER_HERO=8;
	const int ARTIFACTS_QUANTITY=171;
	const int HEROES_QUANTITY=156;
	const int SPELLS_QUANTITY=70;
	const int PRIMARY_SKILLS=4;
	const int NEUTRAL_PLAYER=255;
	const int NAMES_PER_TOWN=16;
	const int CREATURES_PER_TOWN = 7; //without upgrades
	const int SPELL_LEVELS = 5;
	const int AVAILABLE_HEROES_PER_PLAYER = 2;
	const int SPELLBOOK_GOLD_COST = 500;
	const int PUZZLE_MAP_PIECES = 48;

	const int BATTLE_PENALTY_DISTANCE = 10; //if the distance is > than this, then shooting stack has distance penalty

	const ui16 BACKPACK_START = 19;
	const int ID_CATAPULT = 3, ID_SELECTION=144, ID_LOCK = 145;

	const int TERRAIN_TYPES=10;
	const int RESOURCE_QUANTITY=8;

}

// Enum declarations
namespace PrimarySkill
{
	enum PrimarySkill { ATTACK, DEFENSE, SPELL_POWER, KNOWLEDGE};
}

namespace EVictoryConditionType
{
	enum EVictoryConditionType { ARTIFACT, GATHERTROOP, GATHERRESOURCE, BUILDCITY, BUILDGRAIL, BEATHERO,
		CAPTURECITY, BEATMONSTER, TAKEDWELLINGS, TAKEMINES, TRANSPORTITEM, WINSTANDARD = 255 };
}

namespace ELossConditionType
{
	enum ELossConditionType { LOSSCASTLE, LOSSHERO, TIMEEXPIRES, LOSSSTANDARD = 255 };
}

namespace EAlignment
{
	enum EAlignment { GOOD, EVIL, NEUTRAL };
}

namespace ETownType
{
	enum ETownType
	{
		ANY = -1,
		CASTLE, RAMPART, TOWER, INFERNO, NECROPOLIS, DUNGEON, STRONGHOLD, FORTRESS, CONFLUX
	};
}

namespace EBuilding
{
	//Quite useful as long as most of building mechanics hardcoded
	// NOTE: all building with completely configurable mechanics will be removed from list
	enum EBuilding
	{
		MAGES_GUILD_1,  MAGES_GUILD_2, MAGES_GUILD_3,     MAGES_GUILD_4,   MAGES_GUILD_5,
		TAVERN,         SHIPYARD,      FORT,              CITADEL,         CASTLE,
		VILLAGE_HALL,   TOWN_HALL,     CITY_HALL,         CAPITOL,         MARKETPLACE,
		RESOURCE_SILO,  BLACKSMITH,    SPECIAL_1,         HORDE_1,         HORDE_1_UPGR,
		SHIP,           SPECIAL_2,     SPECIAL_3,         SPECIAL_4,       HORDE_2,
		HORDE_2_UPGR,   GRAIL,         EXTRA_TOWN_HALL,   EXTRA_CITY_HALL, EXTRA_CAPITOL,
		DWELL_FIRST=30, DWELL_LAST=36, DWELL_UP_FIRST=37, DWELL_UP_LAST=43,

		//Special buildings for towns.
		LIGHTHOUSE  = SPECIAL_1,
		STABLES     = SPECIAL_2, //Castle
		BROTHERHOOD = SPECIAL_3,

		MYSTIC_POND         = SPECIAL_1,
		FOUNTAIN_OF_FORTUNE = SPECIAL_3, //Rampart
		TREASURY            = SPECIAL_4,

		ARTIFACT_MERCHANT = SPECIAL_1,
		LOOKOUT_TOWER     = SPECIAL_2, //Tower
		LIBRARY           = SPECIAL_3,
		WALL_OF_KNOWLEDGE = SPECIAL_4,

		STORMCLOUDS   = SPECIAL_2,
		CASTLE_GATE   = SPECIAL_3, //Inferno
		ORDER_OF_FIRE = SPECIAL_4,

		COVER_OF_DARKNESS    = SPECIAL_1,
		NECROMANCY_AMPLIFIER = SPECIAL_2, //Necropolis
		SKELETON_TRANSFORMER = SPECIAL_3,

		//ARTIFACT_MERCHANT - same ID as in tower
		MANA_VORTEX      = SPECIAL_2,
		PORTAL_OF_SUMMON = SPECIAL_3, //Dungeon
		BATTLE_ACADEMY   = SPECIAL_4,

		ESCAPE_TUNNEL     = SPECIAL_1,
		FREELANCERS_GUILD = SPECIAL_2, //Stronghold
		BALLISTA_YARD     = SPECIAL_3,
		HALL_OF_VALHALLA  = SPECIAL_4,

		CAGE_OF_WARLORDS = SPECIAL_1,
		GLYPHS_OF_FEAR   = SPECIAL_2, // Fortress
		BLOOD_OBELISK    = SPECIAL_3,

		//ARTIFACT_MERCHANT - same ID as in tower
		MAGIC_UNIVERSITY = SPECIAL_2, // Conflux
	};
}

namespace EBuildingState
{
	enum EBuildingState
	{
		HAVE_CAPITAL, NO_WATER, FORBIDDEN, ADD_MAGES_GUILD, ALREADY_PRESENT, CANT_BUILD_TODAY,
		NO_RESOURCES, ALLOWED, PREREQUIRES, BUILDING_ERROR
	};
}

namespace ESpellCastProblem
{
	enum ESpellCastProblem
	{
		OK, NO_HERO_TO_CAST_SPELL, ALREADY_CASTED_THIS_TURN, NO_SPELLBOOK, ANOTHER_ELEMENTAL_SUMMONED,
		HERO_DOESNT_KNOW_SPELL, NOT_ENOUGH_MANA, ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL,
		SECOND_HEROS_SPELL_IMMUNITY, SPELL_LEVEL_LIMIT_EXCEEDED, NO_SPELLS_TO_DISPEL,
		NO_APPROPRIATE_TARGET, STACK_IMMUNE_TO_SPELL, WRONG_SPELL_TARGET, ONGOING_TACTIC_PHASE,
		MAGIC_IS_BLOCKED, //For Orb of Inhibition and similar - no casting at all
		INVALID
	};
}

namespace ECastingMode
{
	enum ECastingMode {HERO_CASTING, AFTER_ATTACK_CASTING, //also includes cast before attack
		MAGIC_MIRROR, CREATURE_ACTIVE_CASTING, ENCHANTER_CASTING};
}

namespace EMarketMode
{
	enum EMarketMode
	{
		RESOURCE_RESOURCE, RESOURCE_PLAYER, CREATURE_RESOURCE, RESOURCE_ARTIFACT, 
		ARTIFACT_RESOURCE, ARTIFACT_EXP, CREATURE_EXP, CREATURE_UNDEAD, RESOURCE_SKILL,
		MARTKET_AFTER_LAST_PLACEHOLDER
	};
}

namespace EBattleStackState
{
	enum EBattleStackState{ALIVE = 180, SUMMONED, CLONED, HAD_MORALE, WAITING, MOVED, DEFENDING, FEAR,
		DRAINED_MANA /*remember to drain mana only once per turn*/};
}

namespace ECommander
{
	enum SecondarySkills {ATTACK, DEFENSE, HEALTH, DAMAGE, SPEED, SPELL_POWER, CASTS, RESISTANCE};
	const int MAX_SKILL_LEVEL = 5;
}

namespace EWallParts
{
	enum EWallParts
	{
		INDESTRUCTIBLE_PART = -2, INVALID = -1,
		KEEP = 0, BOTTOM_TOWER, BOTTOM_WALL, BELOW_GATE, OVER_GATE, UPPER_WAL, UPPER_TOWER, GATE,

		PARTS_COUNT
	};
}

namespace EWallState
{
	enum
	{
		NONE, //no wall
		INTACT,
		DAMAGED,
		DESTROYED
	};
}

namespace Obj
{
	enum
	{
		ALTAR_OF_SACRIFICE = 2,
		ANCHOR_POINT = 3,
		ARENA = 4,
		ARTIFACT = 5,
		PANDORAS_BOX = 6,
		BLACK_MARKET = 7,
		BOAT = 8,
		BORDERGUARD = 9,
		KEYMASTER = 10,
		BUOY = 11,
		CAMPFIRE = 12,
		CARTOGRAPHER = 13,
		SWAN_POND = 14,
		COVER_OF_DARKNESS = 15,
		CREATURE_BANK = 16,
		CREATURE_GENERATOR1 = 17,
		CREATURE_GENERATOR2 = 18,
		CREATURE_GENERATOR3 = 19,
		CREATURE_GENERATOR4 = 20,
		CURSED_GROUND1 = 21,
		CORPSE = 22,
		MARLETTO_TOWER = 23,
		DERELICT_SHIP = 24,
		DRAGON_UTOPIA = 25,
		EVENT = 26,
		EYE_OF_MAGI = 27,
		FAERIE_RING = 28,
		FLOTSAM = 29,
		FOUNTAIN_OF_FORTUNE = 30,
		FOUNTAIN_OF_YOUTH = 31,
		GARDEN_OF_REVELATION = 32,
		GARRISON = 33,
		HERO = 34,
		HILL_FORT = 35,
		HUT_OF_MAGI = 37,
		IDOL_OF_FORTUNE = 38,
		LEAN_TO = 39,
		LIBRARY_OF_ENLIGHTENMENT = 41,
		MONOLITH1 = 43,
		MONOLITH2 = 44,
		MONOLITH3 = 45,
		MAGIC_PLAINS1 = 46,
		SCHOOL_OF_MAGIC = 47,
		MAGIC_WELL = 49,
		MERCENARY_CAMP = 51,
		MERMAID = 52,
		MINE = 53,
		MONSTER = 54,
		MYSTICAL_GARDEN = 55,
		OASIS = 56,
		OBELISK = 57,
		REDWOOD_OBSERVATORY = 58,
		OCEAN_BOTTLE = 59,
		PILLAR_OF_FIRE = 60,
		STAR_AXIS = 61,
		PRISON = 62,
		PYRAMID = 63,
		RALLY_FLAG = 64,
		RANDOM_ART = 65,
		RANDOM_TREASURE_ART = 66,
		RANDOM_MINOR_ART = 67,
		RANDOM_MAJOR_ART = 68,
		RANDOM_RELIC_ART = 69,
		RANDOM_HERO = 70,
		RANDOM_MONSTER = 71,		
		RANDOM_MONSTER_L1 = 72,
		RANDOM_MONSTER_L2 = 73,
		RANDOM_MONSTER_L3 = 74,
		RANDOM_MONSTER_L4 = 75,
		RANDOM_RESOURCE = 76,
		RANDOM_TOWN = 77,
		REFUGEE_CAMP = 78,
		RESOURCE = 79,
		SANCTUARY = 80,
		SCHOLAR = 81,
		SEA_CHEST = 82,
		SEER_HUT = 83,
		CRYPT = 84,
		SHIPWRECK = 85,
		SHIPWRECK_SURVIVOR = 86,
		SHIPYARD = 87,
		SHRINE_OF_MAGIC_THOUGHT = 90,
		SPELL_SCROLL = 93,
		STABLES = 94,
		TAVERN = 95,
		TEMPLE = 96,
		DEN_OF_THIEVES = 97,
		TOWN = 98,
		TRADING_POST = 99,
		LEARNING_STONE = 100,
		TREASURE_CHEST = 101,
		TREE_OF_KNOWLEDGE = 102,
		SUBTERRANEAN_GATE = 103,
		UNIVERSITY = 104,
		WAGON = 105,
		WAR_MACHINE_FACTORY = 106,
		SCHOOL_OF_WAR = 107,
		WARRIORS_TOMB = 108,
		WATER_WHEEL = 109,
		WATERING_HOLE = 110,
		WHIRLPOOL = 111,
		WINDMILL = 112,
		HOLE = 124,
		RANDOM_MONSTER_L5 = 162,
		RANDOM_MONSTER_L6 = 163,
		RANDOM_MONSTER_L7 = 164,
		BORDER_GATE = 212,
		FREELANCERS_GUILD = 213,
		HERO_PLACEHOLDER = 214,
		QUEST_GUARD = 215,
		RANDOM_DWELLING = 216,
		RANDOM_DWELLING_LVL = 217, //subtype = creature level
		RANDOM_DWELLING_FACTION = 218, //subtype = faction
		GARRISON2 = 219,
		ABANDONED_MINE = 220,
		TRADING_POST_SNOW = 221,
		CLOVER_FIELD = 222,
		CURSED_GROUND2 = 223,
		EVIL_FOG = 224,
		FAVORABLE_WINDS = 225,
		FIERY_FIELDS = 226,
		HOLY_GROUNDS = 227,
		LUCID_POOLS = 228,
		MAGIC_CLOUDS = 229,
		MAGIC_PLAINS2 = 230,
		ROCKLANDS = 231,
	};
}

namespace SecSkillLevel
{
	enum SecSkillLevel
	{
		NONE,
		BASIC,
		ADVANCED,
		EXPERT
	};
}

//follows ERM BI (battle image) format
namespace BattlefieldBI
{
	enum
	{
		NONE = -1,
		COASTAL,
		//Discrepency from ERM BI description - we have magic plains and cursed gronds swapped
		MAGIC_PLAINS,
		CURSED_GROUND,
		//
		HOLY_GROUND,
		EVIL_FOG,
		CLOVER_FIELD,
		LUCID_POOLS,
		FIERY_FIELDS,
		ROCKLANDS,
		MAGIC_CLOUDS,
	};
}

// Typedef declarations
typedef si8 TFaction;
typedef si64 TExpType;
typedef ui32 TSpell;
typedef std::pair<ui32, ui32> TDmgRange;
typedef ui8 TBonusType;
typedef si32 TBonusSubtype;
typedef si32 TSlot;
typedef si32 TQuantity;
typedef si32 TArtifactID;
typedef si32 TArtifactInstanceID;
typedef ui32 TCreature; //creature id
typedef ui8 TPlayerColor;
