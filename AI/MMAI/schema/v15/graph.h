/*
 * graph.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace MMAI::Schema::V15::Graph
{
enum class ElementType : uint8_t
{
	NODE_GLOBAL,
	NODE_PLAYER,
	NODE_UNIT,
	NODE_HEX,
	NODE_ACTION,

	EDGE_GLOBAL_TO_PLAYER,
	EDGE_PLAYER_TO_GLOBAL,

	EDGE_GLOBAL_TO_UNIT,
	EDGE_UNIT_TO_GLOBAL,

	EDGE_GLOBAL_TO_HEX,
	EDGE_HEX_TO_GLOBAL,

	EDGE_GLOBAL_TO_ACTION,
	// EDGE_ACTION_TO_GLOBAL,

	EDGE_PLAYER_OWNS_UNIT,
	EDGE_UNIT_OWNED_BY_PLAYER,

	EDGE_UNIT_OCCUPIES_HEX,
	EDGE_HEX_OCCUPIED_BY_UNIT,

	EDGE_ACTION_BY_UNIT,
	EDGE_UNIT_HAS_ACTION,

	EDGE_HEX_ADJACENT_HEX,
	EDGE_UNIT_ACTS_BEFORE_UNIT,
	EDGE_UNIT_MELEE_DMG_UNIT, // regardless if reachable
	EDGE_UNIT_SHOOT_DMG_UNIT, // regardless if blocked
	EDGE_UNIT_BLOCKS_UNIT,

	EDGE_ACTION_ENDS_AT_HEX,
	EDGE_HEX_IS_END_OF_ACTION,

	EDGE_ACTION_BLOCKS_UNIT,
	EDGE_UNIT_BLOCKED_BY_ACTION,

	// XXX: the below were originally reversed
	// However, these edges are quite dense (can be thousands)
	// and should be uni-directional
	// The direction should be towards the action => rename them

	// EDGE_ACTION_EXPOSES_TO_MELEE_FROM_UNIT,
	EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION,

	// EDGE_ACTION_EXPOSES_TO_SHOOT_FROM_UNIT,    // v=ranged penalty
	EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION,

	// EDGE_ACTION_MELEES_UNIT, // v=primary target (e.g. for dragon breath, 3-headed attack, etc.)
	EDGE_UNIT_IS_MELEED_BY_ACTION,

	// EDGE_ACTION_SHOOTS_UNIT, // v=primary target (e.g. for fireball)
	EDGE_UNIT_IS_SHOT_BY_ACTION,

	// EDGE_ACTION_ENABLES_MELEE_AT_UNIT,
	EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION,

	// EDGE_ACTION_ENABLES_SHOOT_AT_UNIT, // v=ranged penalty
	EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION,

	// // can explode to 350K for 14 archangels
	// // => present only for active action nodes
	// EDGE_ACTION_ENABLES_MELEE_AT_HEX,
	EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION,

	// EDGE_ACTION_ENABLES_SHOOT_AT_HEX, // v=ranged penalty
	EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION,

	_count
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define BLANK_ENUM_DEF(name)  \
	enum class name : uint8_t \
	{                         \
		_count                \
	}

namespace NodeAttributes
{
	enum class Global : uint8_t
	{
		BATTLE_WINNER, //             0=left, 1=right (NA = battle not finished)
		BATTLE_ROUND,
		HAS_UPPER_TOWER,
		HAS_MIDDLE_TOWER,
		HAS_BOTTOM_TOWER,
		HAS_GATE_CORPSE,
		HAS_BRIDGE_CORPSE,

		_count
	};

	enum class Player : uint8_t
	{
		BATTLE_SIDE, //           0=left, 1=right
		IS_ACTIVE,
		ARMY_VALUE_NOW_REL0, //   side_army_value_now / global_value_at_start
		ARMY_VALUE_NOW_REL, //    side_army_value_now / global_value_now
		ARMY_HP_NOW_REL, //       side_army_hp_now / global_hp_now
		VALUE_KILLED_NOW_REL, //  left_value_killed_this_turn / global_value_last_turn
		VALUE_LOST_NOW_REL, //    left_value_lost_this_turn / global_value_last_turn
		DMG_DEALT_NOW_REL, //     left_dmg_dealt_this_turn / global_hp_last_turn
		DMG_RECEIVED_NOW_REL, //  left_dmg_taken_this_turn / global_hp_last_turn

		_count
	};

	enum class Unit : uint8_t
	{
		VALUE_REL, // stack_value_now / global_value_now
		SHOTS,
		DMG_UNCERTAINTY, // dmg uncertainty, e.g. naga=0, orc(1x)=0.25, orc(10x)=0.08
		IS_ACTIVE,
		IS_ENEMY,
		IS_SLEEPING,
		IS_WAR_MACHINE,
		// IS_CLONED,   // these are useful, but not in the current
		// IS_SUMMONED, // training setup where they are never true
		HAS_ADDITIONAL_ATTACK,
		HAS_ALL_AROUND_ATTACK,
		HAS_BLOCKS_RETALIATION,
		HAS_DEATH_CLOUD,
		HAS_DOUBLE_DAMAGE_CHANCE, // v=chance
		HAS_FIREBALL,
		HAS_FLYING,
		HAS_LIFE_DRAIN,
		HAS_NON_LIVING,
		HAS_NO_MELEE_PENALTY,
		HAS_RETURN_AFTER_STRIKE,
		HAS_THREE_HEADED_ATTACK,
		HAS_TWO_HEX_ATTACK_BREATH,
		HAS_AGE,
		HAS_AGE_ATTACK, //      v=chance
		HAS_BIND, //            v=rounds
		HAS_BIND_ATTACK, //     v=chance
		HAS_BLIND, //           v=rounds
		HAS_BLIND_ATTACK, //    v=chance
		HAS_CURSE, //           v=rounds
		HAS_CURSE_ATTACK, //    v=chance
		HAS_DISPEL_ATTACK, //   v=chance
		HAS_PETRIFY, //         v=rounds
		HAS_PETRIFY_ATTACK, //  v=chance
		HAS_POISON, //          v=rounds
		HAS_POISON_ATTACK, //   v=chance
		HAS_WEAKNESS, //        v=rounds
		HAS_WEAKNESS_ATTACK, // v=chance

		_count
	};

	enum class Hex : uint8_t
	{
		Y_COORD,
		X_COORD,

		IS_PASSABLE, //      empty/mine/firewall/gate(open)/gate(closed,defender), ...
		IS_STOPPING, //      moat/quicksand
		IS_DAMAGING_L, //    moat/mine/firewall
		IS_DAMAGING_R, //    moat/mine/firewall
		IS_SIEGE_GATE, //    the two gate hexes
		IS_SIEGE_BRIDGE, //  the bridge hex
		IS_OBSTACLE, //      permanent obstacles/indestructible walls/space between boats, ...
		WALL_HEALTH, //      v=1..3 (destructible walls only), v=0 (no wall, or destroyed)

		_count
	};

	enum class Action : uint8_t
	{
		ACTION_TYPE,
		IS_ACTIVE,

		_count
	};
}

namespace EdgeAttributes
{
	BLANK_ENUM_DEF(Global_To_Player);
	BLANK_ENUM_DEF(Player_To_Global);

	BLANK_ENUM_DEF(Global_To_Unit);
	BLANK_ENUM_DEF(Unit_To_Global);

	BLANK_ENUM_DEF(Global_To_Hex);
	BLANK_ENUM_DEF(Hex_To_Global);

	BLANK_ENUM_DEF(Global_To_Action);

	BLANK_ENUM_DEF(Player_Owns_Unit);
	BLANK_ENUM_DEF(Unit_OwnedBy_Player);

	enum class Hex_Adjacent_Hex : uint8_t
	{
		DIRECTION,
		_count
	};

	BLANK_ENUM_DEF(Unit_Blocks_Unit);
	BLANK_ENUM_DEF(Unit_Occupies_Hex);
	BLANK_ENUM_DEF(Hex_OccupiedBy_Unit);

	enum class Unit_ActsBefore_Unit : uint8_t
	{
		TIMES,
		_count
	};

	enum class Unit_MeleeDmg_Unit : uint8_t
	{
		ESTIMATED_ATTACKER_HPDIFF_REL_SELF,
		ESTIMATED_ATTACKER_HPDIFF_REL_BF,
		ESTIMATED_DEFENDER_HPDIFF_REL_SELF,
		ESTIMATED_DEFENDER_HPDIFF_REL_BF,
		ESTIMATED_NET_VALUE_REL_BF, // value from attacker's POV
		_count
	};

	enum class Unit_ShootDmg_Unit : uint8_t
	{
		ESTIMATED_ATTACKER_HPDIFF_REL_SELF,
		ESTIMATED_ATTACKER_HPDIFF_REL_BF,
		ESTIMATED_DEFENDER_HPDIFF_REL_SELF,
		ESTIMATED_DEFENDER_HPDIFF_REL_BF,
		ESTIMATED_NET_VALUE_REL_BF, // value from attacker's POV
		_count
	};

	BLANK_ENUM_DEF(Action_By_Unit);
	BLANK_ENUM_DEF(Unit_Has_Action);
	BLANK_ENUM_DEF(Action_Blocks_Unit);
	BLANK_ENUM_DEF(Unit_BlockedBy_Action);

	enum class Action_EndsAt_Hex : uint8_t
	{
		IS_REAR,
		_count
	};

	enum class Hex_IsEndOf_Action : uint8_t
	{
		IS_REAR,
		_count
	};

	BLANK_ENUM_DEF(Unit_BecomesMeleeThreatAfter_Action);

	enum class Unit_BecomesShootThreatAfter_Action : uint8_t
	{
		DMG_MULT, // 1=full dmg
		_count
	};

	enum class Unit_IsMeleedBy_Action : uint8_t
	{
		IS_PRIMARY_TARGET, // e.g. for dragons
		_count
	};

	enum class Unit_IsShotBy_Action : uint8_t
	{
		IS_PRIMARY_TARGET, // e.g. for magogs
		_count
	};

	BLANK_ENUM_DEF(Unit_BecomesMeleeTargetAfter_Action);

	enum class Unit_BecomesShootTargetAfter_Action : uint8_t
	{
		DMG_MULT, // 1=full dmg
		_count
	};

	BLANK_ENUM_DEF(Hex_BecomesMeleeTargetAfter_Action);

	enum class Hex_BecomesShootTargetAfter_Action : uint8_t
	{
		DMG_MULT, // 1=full dmg
		_count
	};

	// # of nodes + # of edges
	static_assert(static_cast<int>(ElementType::_count) == 5 + 30);
};

class INode
{
public:
	virtual ElementType getType() const = 0;
	virtual std::vector<int> rawAttributes() const = 0;
	virtual int encode(std::span<float> out) const = 0;
	virtual std::string name() const = 0;
	virtual ~INode() = default;
};

using Endpoints = std::pair<const INode *, const INode *>;

class IEdge
{
public:
	virtual ElementType getType() const = 0;
	virtual std::vector<int> rawAttributes() const = 0;
	virtual int encode(std::span<float> out) const = 0;
	virtual std::string name() const = 0;
	virtual Endpoints endpoints() const = 0;
	virtual ~IEdge() = default;
};

class IGraph
{
public:
	virtual std::vector<const INode *> getNodes(ElementType t) const = 0;
	virtual std::vector<const IEdge *> getEdges(ElementType t) const = 0;
	virtual int64_t getNodeIndex(const INode *) const = 0;
	virtual int64_t getEdgeIndex(const IEdge *) const = 0;
	virtual const INode * getNode(ElementType t, std::size_t ind) const = 0;
	virtual std::vector<int64_t> getActiveActionIds() const = 0;
	virtual ~IGraph() = default;
};
} // namespace
