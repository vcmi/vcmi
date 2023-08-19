/*
 * GameConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

enum class PrimarySkill : int8_t
{
	NONE = -1,
	ATTACK,
	DEFENSE,
	SPELL_POWER,
	KNOWLEDGE,
	EXPERIENCE = 4 //for some reason changePrimSkill uses it
};

enum class EAlignment : int8_t
{
	GOOD,
	EVIL,
	NEUTRAL
};

namespace BuildingSubID
{
	enum EBuildingSubID
	{
		DEFAULT = -50,
		NONE = -1,
		STABLES,
		BROTHERHOOD_OF_SWORD,
		CASTLE_GATE,
		CREATURE_TRANSFORMER,
		MYSTIC_POND,
		FOUNTAIN_OF_FORTUNE,
		ARTIFACT_MERCHANT,
		LOOKOUT_TOWER,
		LIBRARY,
		MANA_VORTEX,
		PORTAL_OF_SUMMONING,
		ESCAPE_TUNNEL,
		FREELANCERS_GUILD,
		BALLISTA_YARD,
		ATTACK_VISITING_BONUS,
		MAGIC_UNIVERSITY,
		SPELL_POWER_GARRISON_BONUS,
		ATTACK_GARRISON_BONUS,
		DEFENSE_GARRISON_BONUS,
		DEFENSE_VISITING_BONUS,
		SPELL_POWER_VISITING_BONUS,
		KNOWLEDGE_VISITING_BONUS,
		EXPERIENCE_VISITING_BONUS,
		LIGHTHOUSE,
		TREASURY,
		CUSTOM_VISITING_BONUS,
		CUSTOM_VISITING_REWARD
	};
}

enum class EMarketMode : int8_t
{
	RESOURCE_RESOURCE, RESOURCE_PLAYER, CREATURE_RESOURCE, RESOURCE_ARTIFACT,
	ARTIFACT_RESOURCE, ARTIFACT_EXP, CREATURE_EXP, CREATURE_UNDEAD, RESOURCE_SKILL,
	MARTKET_AFTER_LAST_PLACEHOLDER
};

enum class EAiTactic : int8_t
{
	NONE = -1,
	RANDOM,
	WARRIOR,
	BUILDER,
	EXPLORER
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

namespace SecSkillLevel
{
	enum SecSkillLevel
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

VCMI_LIB_NAMESPACE_END
