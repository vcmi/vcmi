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
	const std::string VCMI_VERSION = "VCMI 0.88b";

	/* 
	 * DATA_DIR contains the game data (Data/, MP3/, ...).
	 * BIN_DIR is where the vcmiclient/vcmiserver binaries reside
	 * LIB_DIR is where the AI libraries reside (linux only)
	 */
	#ifdef _WIN32
		const std::string DATA_DIR = ".";
		const std::string BIN_DIR = ".";
		const std::string LIB_DIR = ".";
		const std::string SERVER_NAME = "VCMI_server.exe";
		const std::string LIB_EXT = "dll";
		const std::string PATH_SEPARATOR = "\\";
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

	const int BOATI_TYPE = 8;
	const int HEROI_TYPE = 34;
	const int TOWNI_TYPE = 98;
	const int SUBTERRANEAN_GATE_TYPE = 103;
	const int CREI_TYPE = 54;
	const int EVENTI_TYPE = 26;

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
	const int RESOURCE_QUANTITY=8;
	const int TERRAIN_TYPES=10;
	const int PRIMARY_SKILLS=4;
	const int NEUTRAL_PLAYER=255;
	const int NAMES_PER_TOWN=16;
	const int CREATURES_PER_TOWN = 7; //without upgrades
	const int MAX_BUILDING_PER_TURN = 1;
	const int SPELL_LEVELS = 5;
	const int CREEP_SIZE = 4000; // neutral stacks won't grow beyond this number
	const int WEEKLY_GROWTH = 10; //percent
	const int AVAILABLE_HEROES_PER_PLAYER = 2;
	const bool DWELLINGS_ACCUMULATE_CREATURES = false;
	const int SPELLBOOK_GOLD_COST = 500;

	const ui16 BACKPACK_START = 19;
	const int ID_CATAPULT = 3, ID_LOCK = 145;

	//game modules
	const bool STACK_EXP = true;
	const bool STACK_ARTIFACT = true; //now toggle for testing
	const bool COMMANDERS = false;
	const bool MITHRIL = false;
}

// Enum declarations
namespace PrimarySkill
{
	enum { ATTACK, DEFENSE, SPELL_POWER, KNOWLEDGE};
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

namespace EBuilding
{
	//Quite useful as long as most of building mechanics hardcoded
	enum EBuilding
	{
		MAGES_GUILD_1,  MAGES_GUILD_2, MAGES_GUILD_3,     MAGES_GUILD_4,   MAGES_GUILD_5, 
		TAVERN,         SHIPYARD,      FORT,              CITADEL,         CASTLE,
		VILLAGE_HALL,   TOWN_HALL,     CITY_HALL,         CAPITOL,         MARKETPLACE,
		RESOURCE_SILO,  BLACKSMITH,    SPECIAL_1,         HORDE_1,         HORDE_1_UPGR,
		SHIP,           SPECIAL_2,     SPECIAL_3,         SPECIAL_4,       HORDE_2,
		HORDE_2_UPGR,   GRAIL,         EXTRA_CITY_HALL,   EXTRA_TOWN_HALL, EXTRA_CAPITOL,
		DWELL_FIRST=30, DWELL_LAST=36, DWELL_UP_FIRST=37, DWELL_UP_LAST=43
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
		NO_APPROPRIATE_TARGET, STACK_IMMUNE_TO_SPELL, WRONG_SPELL_TARGET, ONGOING_TACTIC_PHASE
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
	enum EBattleStackState{ALIVE = 180, SUMMONED, CLONED, HAD_MORALE, WAITING, MOVED, DEFENDING, FEAR};
}

namespace ECommander
{
	enum SecondarySkills {ATTACK, DEFENSE, HEALTH, DAMAGE, SPEED, SPELL_POWER};
}

namespace Obj
{
	enum
	{
		BOAT = 8,
		CREATURE_BANK = 16,
		CREATURE_GENERATOR1 = 17,
		DERELICT_SHIP = 24,
		DRAGON_UTOPIA = 25,
		GARRISON = 33,
		LIBRARY_OF_ENLIGHTENMENT = 41,
		MONOLITH1 = 43,
		MONOLITH2 = 44,
		MONOLITH3 = 45,
		SCHOOL_OF_MAGIC = 47,
		MINE = 53,
		MONSTER = 54,
		OBELISK = 57,
		PYRAMID = 63,
		CRYPT = 84,
		SHIPWRECK = 85,
		STABLES = 94,
		TRADING_POST = 99,
		SUBTERRANEAN_GATE = 103,
		UNIVERSITY = 104,
		SCHOOL_OF_WAR = 107,
		WHIRLPOOL = 111,
		BORDER_GATE = 212,
		GARRISON2 = 219,
	};
}

// Typedef declarations
typedef si64 expType;
typedef ui32 TSpell;
typedef std::pair<ui32, ui32> TDmgRange;
typedef ui8 TBonusType;
typedef si32 TBonusSubtype;
typedef si32 TSlot;
typedef si32 TQuantity; 
typedef ui32 TCreature; //creature id
