/*
 * Enumerations.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class EAlignment : int8_t
{
	ANY = -1,
	GOOD = 0,
	EVIL,
	NEUTRAL
};

namespace BuildingSubID
{
	enum EBuildingSubID
	{
		DEFAULT = -50,
		NONE = -1,
		CASTLE_GATE,
		MYSTIC_POND,
		LIBRARY,
		PORTAL_OF_SUMMONING,
		ESCAPE_TUNNEL,
		TREASURY,
		BANK,
		AURORA_BOREALIS,
		DEITY_OF_FIRE
	};
}

enum class EMarketMode : int8_t
{
	RESOURCE_RESOURCE, RESOURCE_PLAYER, CREATURE_RESOURCE, RESOURCE_ARTIFACT,
	ARTIFACT_RESOURCE, ARTIFACT_EXP, CREATURE_EXP, CREATURE_UNDEAD, RESOURCE_SKILL,
	MARKET_AFTER_LAST_PLACEHOLDER
};

enum class EAiTactic : int8_t
{
	NONE = -1,
	RANDOM = 0,
	WARRIOR = 1,
	BUILDER = 2,
	EXPLORER = 3
};

enum class EBuildingState : int8_t
{
	HAVE_CAPITAL, NO_WATER, FORBIDDEN, ADD_MAGES_GUILD, ALREADY_PRESENT, CANT_BUILD_TODAY,
	NO_RESOURCES, ALLOWED, PREREQUIRES, MISSING_BASE, BUILDING_ERROR, TOWN_NOT_OWNED
};

enum class ESpellCastProblem : int8_t
{
	OK, NO_HERO_TO_CAST_SPELL, CASTS_PER_TURN_LIMIT, NO_SPELLBOOK,
	HERO_DOESNT_KNOW_SPELL, NOT_ENOUGH_MANA, ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL,
	SPELL_LEVEL_LIMIT_EXCEEDED, NO_SPELLS_TO_DISPEL,
	NO_APPROPRIATE_TARGET, STACK_IMMUNE_TO_SPELL, WRONG_SPELL_TARGET, ONGOING_TACTIC_PHASE,
	MAGIC_IS_BLOCKED, //For Orb of Inhibition and similar - no casting at all
	INVALID
};

namespace ECommander
{
	enum SecondarySkills {ATTACK, DEFENSE, HEALTH, DAMAGE, SPEED, SPELL_POWER, CASTS, RESISTANCE};
	const int MAX_SKILL_LEVEL = 5;
}

enum class EWallPart : int8_t
{
	INDESTRUCTIBLE_PART_OF_GATE = -3, INDESTRUCTIBLE_PART = -2, INVALID = -1,
	KEEP = 0, BOTTOM_TOWER, BOTTOM_WALL, BELOW_GATE, OVER_GATE, UPPER_WALL, UPPER_TOWER, GATE,
	PARTS_COUNT /* This constant SHOULD always stay as the last item in the enum. */
};

enum class EWallState : int8_t
{
	NONE = -1, //no wall
	DESTROYED,
	DAMAGED,
	INTACT,
	REINFORCED, // walls in towns with castle
};

enum class EGateState : int8_t
{
	NONE,
	CLOSED,
	BLOCKED, // gate is blocked in closed state, e.g. by creature
	OPENED,
	DESTROYED
};

enum class ETileType : int8_t
{
	FREE,
	POSSIBLE,
	BLOCKED,
	USED
};

enum class ETeleportChannelType : int8_t
{
	IMPASSABLE,
	BIDIRECTIONAL,
	UNIDIRECTIONAL,
	MIXED
};

namespace MasteryLevel
{
	enum Type
	{
		NONE,
		BASIC,
		ADVANCED,
		EXPERT,
		LEVELS_SIZE
	};
}

enum class Date : int8_t
{
	DAY = 0,
	DAY_OF_WEEK = 1,
	WEEK = 2,
	MONTH = 3,
	DAY_OF_MONTH
};

enum class EActionType : int8_t
{
	NO_ACTION,

	END_TACTIC_PHASE,
	RETREAT,
	SURRENDER,

	HERO_SPELL,

	WALK,
	WAIT,
	DEFEND,
	WALK_AND_ATTACK,
	SHOOT,
	CATAPULT,
	MONSTER_SPELL,
	BAD_MORALE,
	STACK_HEAL,
	WALK_AND_CAST,
};

enum class EDiggingStatus : int8_t
{
	UNKNOWN = -1,
	CAN_DIG = 0,
	LACK_OF_MOVEMENT,
	WRONG_TERRAIN,
	TILE_OCCUPIED,
	BACKPACK_IS_FULL
};

enum class EPlayerStatus : int8_t
{
	WRONG = -1,
	INGAME,
	LOSER,
	WINNER
};

enum class PlayerRelations : int8_t
{
	ENEMIES,
	ALLIES,
	SAME_PLAYER
};

enum class EMetaclass : int8_t
{
	INVALID = 0,
	ARTIFACT,
	CREATURE,
	FACTION,
	EXPERIENCE,
	HERO,
	HEROCLASS,
	LUCK,
	MANA,
	MORALE,
	MOVEMENT,
	OBJECT,
	PRIMARY_SKILL,
	SECONDARY_SKILL,
	SPELL,
	RESOURCE
};

enum class EHealLevel: int8_t
{
	HEAL,
	RESURRECT,
	OVERHEAL
};

enum class EHealPower : int8_t
{
	ONE_BATTLE,
	PERMANENT
};

enum class EBattleResult : int8_t
{
	NORMAL = 0,
	ESCAPE = 1,
	SURRENDER = 2,
};

enum class ETileVisibility : int8_t // Fog of war change
{
	HIDDEN,
	REVEALED
};

enum class EArmyFormation : int8_t
{
	LOOSE,
	TIGHT
};

enum class EMovementMode : int8_t
{
	STANDARD,
	DIMENSION_DOOR,
	MONOLITH,
	CASTLE_GATE,
	TOWN_PORTAL,
};

enum class EMapLevel : int8_t
{
	ANY = -1,
	SURFACE = 0,
	UNDERGROUND = 1
};

enum class EWeekType : int8_t
{
	FIRST_WEEK,
	NORMAL,
	DOUBLE_GROWTH,
	BONUS_GROWTH,
	DEITYOFFIRE,
	PLAGUE
};

enum class ColorScheme : int8_t
{
	NONE,
	KEEP,
	GRAYSCALE,
	H2_SCHEME
};

enum class ChangeValueMode : int8_t
{
	RELATIVE,
	ABSOLUTE
};

VCMI_LIB_NAMESPACE_END
