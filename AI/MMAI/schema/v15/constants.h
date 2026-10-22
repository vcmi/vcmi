/*
 * constants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <stdexcept>
#include <string>
#include <tuple>

#include "schema/base.h"
#include "schema/v15/graph.h"
#include "schema/v15/types.h"
#include "schema/v15/util.h"

namespace MMAI::Schema::V15
{

// Convenience definitions which do not need to be exported
namespace X
{
	inline constexpr auto CAT = Encoding::CATEGORICAL;
	inline constexpr auto LIN = Encoding::LINNORM;
	inline constexpr auto RAW = Encoding::RAW;

	using ET = Graph::ElementType;
}

/*
 * Compile-time constructor for E4H and E4S tuples
 * https://stackoverflow.com/a/23784921
 */
template<typename T>
constexpr std::tuple<T, Encoding, int, int> E4(T a, Encoding e, int vmax)
{
	switch(e)
	{
		// "0" is a category => vmax+1 categories
		case X::CAT:
			return {a, e, vmax + 1, vmax};
		case X::LIN:
		case X::RAW:
			return {a, e, 1, vmax};
		default:
			throw std::runtime_error("Unexpected encoding: " + std::to_string(EI(e)));
	}
}

// 0-6 regular; 7=war machines; 8=other (summoned, commander, etc.)
constexpr int STACK_SLOT_WARMACHINES = 7;
constexpr int STACK_SLOT_SPECIAL = 8;
constexpr int STACK_QUEUE_SIZE = 30;
constexpr auto MAX_ROUNDS = 30;

namespace detail
{
	template<typename AttrType>
	struct EncodingTraitsBase
	{
		/*
		 * attr_enc_schema_type is the `{a, e, n, vmax}` tuple, where:
		 *   a=attribute
		 *   e=encoding
		 *   n=size
		 *   vmax=max_value
		 */
		using A = AttrType;
		using attr_enc_schema_type = std::tuple<AttrType, Encoding, int, int>;
		using encoding_type = std::array<attr_enc_schema_type, EI(AttrType::_count)>;
		static constexpr std::size_t attr_count = EI(AttrType::_count);
	};
}

template<typename AttrType>
struct EncodingTraits;

template<>
struct EncodingTraits<Graph::NodeAttributes::Global> : detail::EncodingTraitsBase<Graph::NodeAttributes::Global>
{
	static constexpr auto element_type = Graph::ElementType::NODE_GLOBAL;
	static constexpr std::string_view name = "Global";
	static constexpr encoding_type encoding = {
		// LS is the correct encoding for BATTLE_ROUND, but since it replaces BATTLE_SIDE
		// which had n=2 => use LE to keep the dimensions unchanged.
		E4(A::BATTLE_WINNER, X::CAT, EI(CombatResult::_count) - 1),
		E4(A::BATTLE_ROUND, X::LIN, MAX_ROUNDS + 1),
		E4(A::HAS_UPPER_TOWER, X::RAW, 1),
		E4(A::HAS_MIDDLE_TOWER, X::RAW, 1),
		E4(A::HAS_BOTTOM_TOWER, X::RAW, 1),
		E4(A::HAS_GATE_CORPSE, X::RAW, 1),
		E4(A::HAS_BRIDGE_CORPSE, X::RAW, 1),
	};
};

template<>
struct EncodingTraits<Graph::NodeAttributes::Player> : detail::EncodingTraitsBase<Graph::NodeAttributes::Player>
{
	static constexpr auto element_type = Graph::ElementType::NODE_PLAYER;
	static constexpr std::string_view name = "Player";
	static constexpr encoding_type encoding = {
		E4(A::BATTLE_SIDE, X::CAT, 1),
		E4(A::IS_ACTIVE, X::RAW, 1),
		E4(A::ARMY_VALUE_NOW_REL0, X::LIN, 1000), //    (army_value_now / global_value_at_start)
		E4(A::ARMY_VALUE_NOW_REL, X::LIN, 1000), //     (army_value_now / global_value_now)
		E4(A::ARMY_HP_NOW_REL, X::LIN, 1000), //        (army_hp_now / global_hp_now)
		E4(A::VALUE_KILLED_NOW_REL, X::LIN, 1000), //   (value_killed_this_turn / global_value_last_turn)
		E4(A::VALUE_LOST_NOW_REL, X::LIN, 1000), //     (value_lost_this_turn / global_value_last_turn)
		E4(A::DMG_DEALT_NOW_REL, X::LIN, 1000), //      (dmg_dealt_this_turn / global_hp_last_turn)
		E4(A::DMG_RECEIVED_NOW_REL, X::LIN, 1000), //   (dmg_received_this_turn / global_hp_last_turn)
	};
};

template<>
struct EncodingTraits<Graph::NodeAttributes::Unit> : detail::EncodingTraitsBase<Graph::NodeAttributes::Unit>
{
	static constexpr Graph::ElementType element_type = Graph::ElementType::NODE_UNIT;
	static constexpr std::string_view name = "Unit";
	static constexpr encoding_type encoding = {
		E4(A::VALUE_REL, X::LIN, 1000), // stack_value_now / global_value_now
		E4(A::SHOTS, X::LIN, 32), // sharpshooter is 32
		E4(A::DMG_UNCERTAINTY, X::LIN, 1000),
		E4(A::IS_ACTIVE, X::RAW, 1),
		E4(A::IS_ENEMY, X::RAW, 1),
		E4(A::IS_SLEEPING, X::RAW, 1),
		E4(A::IS_WAR_MACHINE, X::RAW, 1),
		E4(A::HAS_ADDITIONAL_ATTACK, X::RAW, 1),
		E4(A::HAS_ALL_AROUND_ATTACK, X::RAW, 1),
		E4(A::HAS_BLOCKS_RETALIATION, X::RAW, 1),
		E4(A::HAS_DEATH_CLOUD, X::RAW, 1),
		E4(A::HAS_DOUBLE_DAMAGE_CHANCE, X::LIN, 1000), // v=chance
		E4(A::HAS_FIREBALL, X::RAW, 1),
		E4(A::HAS_FLYING, X::RAW, 1),
		E4(A::HAS_LIFE_DRAIN, X::LIN, 1000),
		E4(A::HAS_NON_LIVING, X::RAW, 1),
		E4(A::HAS_NO_MELEE_PENALTY, X::RAW, 1),
		E4(A::HAS_RETURN_AFTER_STRIKE, X::RAW, 1),
		E4(A::HAS_THREE_HEADED_ATTACK, X::RAW, 1),
		E4(A::HAS_TWO_HEX_ATTACK_BREATH, X::RAW, 1),
		E4(A::HAS_AGE, X::RAW, 3), // 			 	v=rounds
		E4(A::HAS_AGE_ATTACK, X::LIN, 1000), //      v=chance
		E4(A::HAS_BIND, X::RAW, 3), //            	v=rounds
		E4(A::HAS_BIND_ATTACK, X::LIN, 1000), //     v=chance
		E4(A::HAS_BLIND, X::RAW, 3), //           	v=rounds
		E4(A::HAS_BLIND_ATTACK, X::LIN, 1000), //    v=chance
		E4(A::HAS_CURSE, X::RAW, 3), //           	v=rounds
		E4(A::HAS_CURSE_ATTACK, X::LIN, 1000), //    v=chance
		E4(A::HAS_DISPEL_ATTACK, X::LIN, 1000), //   v=chance
		E4(A::HAS_PETRIFY, X::RAW, 3), //         	v=rounds
		E4(A::HAS_PETRIFY_ATTACK, X::LIN, 1000), //  v=chance
		E4(A::HAS_POISON, X::RAW, 3), //          	v=rounds
		E4(A::HAS_POISON_ATTACK, X::LIN, 1000), //   v=chance
		E4(A::HAS_WEAKNESS, X::RAW, 3), //        	v=rounds
		E4(A::HAS_WEAKNESS_ATTACK, X::LIN, 1000), // v=chance
	};
};

template<>
struct EncodingTraits<Graph::NodeAttributes::Hex> : detail::EncodingTraitsBase<Graph::NodeAttributes::Hex>
{
	static constexpr auto element_type = Graph::ElementType::NODE_HEX;
	static constexpr std::string_view name = "Hex";
	static constexpr encoding_type encoding = {
		E4(A::Y_COORD, X::CAT, 10),
		E4(A::X_COORD, X::CAT, 14),
		E4(A::IS_PASSABLE, X::RAW, 1),
		E4(A::IS_STOPPING, X::RAW, 1),
		E4(A::IS_DAMAGING_L, X::RAW, 1),
		E4(A::IS_DAMAGING_R, X::RAW, 1),
		E4(A::IS_SIEGE_GATE, X::RAW, 1),
		E4(A::IS_SIEGE_BRIDGE, X::RAW, 1),
		E4(A::IS_OBSTACLE, X::RAW, 1),
		E4(A::WALL_HEALTH, X::LIN, EI(WallHP::_count)),
	};
};

template<>
struct EncodingTraits<Graph::NodeAttributes::Action> : detail::EncodingTraitsBase<Graph::NodeAttributes::Action>
{
	static constexpr auto element_type = Graph::ElementType::NODE_ACTION;
	static constexpr std::string_view name = "Action";
	static constexpr encoding_type encoding = {
		E4(A::ACTION_TYPE, X::CAT, EI(ActionType::_count)),
		E4(A::IS_ACTIVE, X::RAW, 1),
	};
};

/*
 * The macro is used for generic edges which have no attributes.
 */

#define GENERIC_EDGE_ENCODING_TRAITS(attr_type, elem_type)                                                                 \
	template<>                                                                                                             \
	struct EncodingTraits<Graph::EdgeAttributes::attr_type> : detail::EncodingTraitsBase<Graph::EdgeAttributes::attr_type> \
	{                                                                                                                      \
		static constexpr auto element_type = Graph::ElementType::elem_type;                                                \
		static constexpr std::string_view name = #attr_type;                                                               \
		static constexpr encoding_type encoding = {};                                                                      \
	}

GENERIC_EDGE_ENCODING_TRAITS(Global_To_Player, EDGE_GLOBAL_TO_PLAYER);
GENERIC_EDGE_ENCODING_TRAITS(Player_To_Global, EDGE_PLAYER_TO_GLOBAL);
GENERIC_EDGE_ENCODING_TRAITS(Global_To_Unit, EDGE_GLOBAL_TO_UNIT);
GENERIC_EDGE_ENCODING_TRAITS(Unit_To_Global, EDGE_UNIT_TO_GLOBAL);
GENERIC_EDGE_ENCODING_TRAITS(Global_To_Hex, EDGE_GLOBAL_TO_HEX);
GENERIC_EDGE_ENCODING_TRAITS(Hex_To_Global, EDGE_HEX_TO_GLOBAL);
GENERIC_EDGE_ENCODING_TRAITS(Global_To_Action, EDGE_GLOBAL_TO_ACTION);
GENERIC_EDGE_ENCODING_TRAITS(Player_Owns_Unit, EDGE_PLAYER_OWNS_UNIT);
GENERIC_EDGE_ENCODING_TRAITS(Unit_OwnedBy_Player, EDGE_UNIT_OWNED_BY_PLAYER);

template<>
struct EncodingTraits<Graph::EdgeAttributes::Hex_Adjacent_Hex> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Hex_Adjacent_Hex>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_HEX_ADJACENT_HEX;
	static constexpr std::string_view name = "Hex_Adjacent_Hex";
	static constexpr encoding_type encoding = {
		E4(A::DIRECTION, X::CAT, 5),
	};
};

GENERIC_EDGE_ENCODING_TRAITS(Unit_Blocks_Unit, EDGE_UNIT_BLOCKS_UNIT);
GENERIC_EDGE_ENCODING_TRAITS(Unit_Occupies_Hex, EDGE_UNIT_OCCUPIES_HEX);
GENERIC_EDGE_ENCODING_TRAITS(Hex_OccupiedBy_Unit, EDGE_HEX_OCCUPIED_BY_UNIT);

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_ActsBefore_Unit> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_ActsBefore_Unit>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_ACTS_BEFORE_UNIT;
	static constexpr std::string_view name = "Unit_ActsBefore_Unit";
	static constexpr encoding_type encoding = {
		E4(A::TIMES, X::LIN, 2),
	};
};

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_MeleeDmg_Unit> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_MeleeDmg_Unit>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_MELEE_DMG_UNIT;
	static constexpr std::string_view name = "Unit_MeleeDmg_Unit";

	static constexpr encoding_type encoding = {
		E4(A::ESTIMATED_ATTACKER_HPDIFF_REL_SELF, X::LIN, 1000),
		E4(A::ESTIMATED_ATTACKER_HPDIFF_REL_BF, X::LIN, 1000),
		E4(A::ESTIMATED_DEFENDER_HPDIFF_REL_SELF, X::LIN, 1000),
		E4(A::ESTIMATED_DEFENDER_HPDIFF_REL_BF, X::LIN, 1000),
		E4(A::ESTIMATED_NET_VALUE_REL_BF, X::LIN, 1000),
	};
};

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_ShootDmg_Unit> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_ShootDmg_Unit>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_SHOOT_DMG_UNIT;
	static constexpr std::string_view name = "Unit_ShootDmg_Unit";
	static constexpr encoding_type encoding = {
		E4(A::ESTIMATED_ATTACKER_HPDIFF_REL_SELF, X::LIN, 1000),
		E4(A::ESTIMATED_ATTACKER_HPDIFF_REL_BF, X::LIN, 1000),
		E4(A::ESTIMATED_DEFENDER_HPDIFF_REL_SELF, X::LIN, 1000),
		E4(A::ESTIMATED_DEFENDER_HPDIFF_REL_BF, X::LIN, 1000),
		E4(A::ESTIMATED_NET_VALUE_REL_BF, X::LIN, 1000),
	};
};

template<>
struct EncodingTraits<Graph::EdgeAttributes::Action_EndsAt_Hex> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Action_EndsAt_Hex>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_ACTION_ENDS_AT_HEX;
	static constexpr std::string_view name = "Action_EndsAt_Hex";
	static constexpr encoding_type encoding = {
		E4(A::IS_REAR, X::RAW, 1),
	};
};

template<>
struct EncodingTraits<Graph::EdgeAttributes::Hex_IsEndOf_Action> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Hex_IsEndOf_Action>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_HEX_IS_END_OF_ACTION;
	static constexpr std::string_view name = "Hex_IsEndOf_Action";
	static constexpr encoding_type encoding = {
		E4(A::IS_REAR, X::RAW, 1),
	};
};

GENERIC_EDGE_ENCODING_TRAITS(Action_By_Unit, EDGE_ACTION_BY_UNIT);
GENERIC_EDGE_ENCODING_TRAITS(Unit_Has_Action, EDGE_UNIT_HAS_ACTION);
GENERIC_EDGE_ENCODING_TRAITS(Action_Blocks_Unit, EDGE_ACTION_BLOCKS_UNIT);
GENERIC_EDGE_ENCODING_TRAITS(Unit_BlockedBy_Action, EDGE_UNIT_BLOCKED_BY_ACTION);
GENERIC_EDGE_ENCODING_TRAITS(Unit_BecomesMeleeThreatAfter_Action, EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION);

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_BecomesShootThreatAfter_Action>
	: detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_BecomesShootThreatAfter_Action>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION;
	static constexpr std::string_view name = "Unit_BecomesShootThreatAfter_Action";
	static constexpr encoding_type encoding = {
		E4(A::DMG_MULT, X::LIN, 1000),
	};
};

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_IsMeleedBy_Action> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_IsMeleedBy_Action>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_IS_MELEED_BY_ACTION;
	static constexpr std::string_view name = "Unit_IsMeleedBy_Action";
	static constexpr encoding_type encoding = {
		E4(A::IS_PRIMARY_TARGET, X::CAT, 1),
	};
};

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_IsShotBy_Action> : detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_IsShotBy_Action>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_IS_SHOT_BY_ACTION;
	static constexpr std::string_view name = "Unit_IsShotBy_Action";
	static constexpr encoding_type encoding = {
		E4(A::IS_PRIMARY_TARGET, X::CAT, 1),
	};
};

GENERIC_EDGE_ENCODING_TRAITS(Unit_BecomesMeleeTargetAfter_Action, EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION);

template<>
struct EncodingTraits<Graph::EdgeAttributes::Unit_BecomesShootTargetAfter_Action>
	: detail::EncodingTraitsBase<Graph::EdgeAttributes::Unit_BecomesShootTargetAfter_Action>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION;
	static constexpr std::string_view name = "Unit_BecomesShootTargetAfter_Action";
	static constexpr encoding_type encoding = {
		E4(A::DMG_MULT, X::LIN, 1000),
	};
};

GENERIC_EDGE_ENCODING_TRAITS(Hex_BecomesMeleeTargetAfter_Action, EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION);

template<>
struct EncodingTraits<Graph::EdgeAttributes::Hex_BecomesShootTargetAfter_Action>
	: detail::EncodingTraitsBase<Graph::EdgeAttributes::Hex_BecomesShootTargetAfter_Action>
{
	static constexpr auto element_type = Graph::ElementType::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION;
	static constexpr std::string_view name = "Hex_BecomesShootTargetAfter_Action";
	static constexpr encoding_type encoding = {
		E4(A::DMG_MULT, X::LIN, 1000),
	};
};

template<typename AttrType>
consteval bool EncodingIsValid()
{
	constexpr const auto & encoding = EncodingTraits<AttrType>::encoding;

	// The explicit asserts here are used for more informative errors
	// (a return value is still needed to flag the problematic attribute type)
	static_assert(UninitializedEncodingAttributes(encoding) == 0, "Found uninitialized elements");
	static_assert(DisarrayedEncodingAttributeIndex(encoding) == -1, "Found wrong element at this index");

	return UninitializedEncodingAttributes(encoding) == 0 && DisarrayedEncodingAttributeIndex(encoding) == -1;
}

static_assert(EncodingIsValid<Graph::NodeAttributes::Global>());
static_assert(EncodingIsValid<Graph::NodeAttributes::Player>());
static_assert(EncodingIsValid<Graph::NodeAttributes::Unit>());
static_assert(EncodingIsValid<Graph::NodeAttributes::Hex>());
static_assert(EncodingIsValid<Graph::NodeAttributes::Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Global_To_Player>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Player_To_Global>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Global_To_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_To_Global>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Global_To_Hex>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Hex_To_Global>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Global_To_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Player_Owns_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_OwnedBy_Player>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Hex_Adjacent_Hex>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_ActsBefore_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_MeleeDmg_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_ShootDmg_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_Blocks_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_Occupies_Hex>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Hex_OccupiedBy_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Action_By_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_Has_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Action_EndsAt_Hex>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Hex_IsEndOf_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Action_Blocks_Unit>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_BlockedBy_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_BecomesMeleeThreatAfter_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_BecomesShootThreatAfter_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_IsMeleedBy_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_IsShotBy_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_BecomesMeleeTargetAfter_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Unit_BecomesShootTargetAfter_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Hex_BecomesMeleeTargetAfter_Action>());
static_assert(EncodingIsValid<Graph::EdgeAttributes::Hex_BecomesShootTargetAfter_Action>());
static_assert(static_cast<int>(Graph::ElementType::_count) == 35);

using NodeType = std::tuple<Graph::ElementType, const char *, int>;

inline constexpr std::array NODE_TYPES{
	NodeType{Graph::ElementType::NODE_GLOBAL, "Global", EncodedSize(EncodingTraits<Graph::NodeAttributes::Global>::encoding)},
	NodeType{Graph::ElementType::NODE_PLAYER, "Player", EncodedSize(EncodingTraits<Graph::NodeAttributes::Player>::encoding)},
	NodeType{Graph::ElementType::NODE_UNIT,   "Unit",   EncodedSize(EncodingTraits<Graph::NodeAttributes::Unit>::encoding)  },
	NodeType{Graph::ElementType::NODE_HEX,    "Hex",    EncodedSize(EncodingTraits<Graph::NodeAttributes::Hex>::encoding)   },
	NodeType{Graph::ElementType::NODE_ACTION, "Action", EncodedSize(EncodingTraits<Graph::NodeAttributes::Action>::encoding)},
};

using EdgeType = std::tuple<Graph::ElementType, const char *, std::pair<Graph::ElementType, Graph::ElementType>, int>;

inline constexpr std::array EDGE_TYPES{
	EdgeType{
			 Graph::ElementType::EDGE_GLOBAL_TO_PLAYER,
			 "To",					  {Graph::ElementType::NODE_GLOBAL, Graph::ElementType::NODE_PLAYER},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Global_To_Player>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_PLAYER_TO_GLOBAL,
			 "To",					  {Graph::ElementType::NODE_PLAYER, Graph::ElementType::NODE_GLOBAL},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Player_To_Global>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_GLOBAL_TO_UNIT,
			 "To",					  {Graph::ElementType::NODE_GLOBAL, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Global_To_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_TO_GLOBAL,
			 "To",					  {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_GLOBAL},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_To_Global>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_GLOBAL_TO_HEX,
			 "To",					  {Graph::ElementType::NODE_GLOBAL, Graph::ElementType::NODE_HEX},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Global_To_Hex>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_HEX_TO_GLOBAL,
			 "To",					  {Graph::ElementType::NODE_HEX, Graph::ElementType::NODE_GLOBAL},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Hex_To_Global>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_GLOBAL_TO_ACTION,
			 "To",					  {Graph::ElementType::NODE_GLOBAL, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Global_To_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_PLAYER_OWNS_UNIT,
			 "Owns",					{Graph::ElementType::NODE_PLAYER, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Player_Owns_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_OWNED_BY_PLAYER,
			 "OwnedBy",				 {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_PLAYER},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_OwnedBy_Player>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_HEX_ADJACENT_HEX,
			 "Adjacent",				{Graph::ElementType::NODE_HEX, Graph::ElementType::NODE_HEX},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Hex_Adjacent_Hex>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_ACTS_BEFORE_UNIT,
			 "ActsBefore",			  {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_ActsBefore_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_MELEE_DMG_UNIT,
			 "MeleeDmg",				{Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_MeleeDmg_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_SHOOT_DMG_UNIT,
			 "ShootDmg",				{Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_ShootDmg_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_BLOCKS_UNIT,
			 "Blocks",				  {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_Blocks_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_OCCUPIES_HEX,
			 "Occupies",				{Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_HEX},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_Occupies_Hex>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_HEX_OCCUPIED_BY_UNIT,
			 "OccupiedBy",			  {Graph::ElementType::NODE_HEX, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Hex_OccupiedBy_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_ACTION_BY_UNIT,
			 "By",					  {Graph::ElementType::NODE_ACTION, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Action_By_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_HAS_ACTION,
			 "Has",					 {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_Has_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_ACTION_ENDS_AT_HEX,
			 "EndsAt",				  {Graph::ElementType::NODE_ACTION, Graph::ElementType::NODE_HEX},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Action_EndsAt_Hex>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_HEX_IS_END_OF_ACTION,
			 "IsEndOf",				 {Graph::ElementType::NODE_HEX, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Hex_IsEndOf_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_ACTION_BLOCKS_UNIT,
			 "Blocks",				  {Graph::ElementType::NODE_ACTION, Graph::ElementType::NODE_UNIT},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Action_Blocks_Unit>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_BLOCKED_BY_ACTION,
			 "BlockedBy",			   {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_BlockedBy_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION,
			 "BecomesMeleeThreatAfter", {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_BecomesMeleeThreatAfter_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION,
			 "BecomesShootThreatAfter", {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_BecomesShootThreatAfter_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_IS_MELEED_BY_ACTION,
			 "IsMeleedBy",			  {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_IsMeleedBy_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_IS_SHOT_BY_ACTION,
			 "IsShotBy",				{Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_IsShotBy_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION,
			 "BecomesMeleeTargetAfter", {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_BecomesMeleeTargetAfter_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION,
			 "BecomesShootTargetAfter", {Graph::ElementType::NODE_UNIT, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Unit_BecomesShootTargetAfter_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION,
			 "BecomesMeleeTargetAfter", {Graph::ElementType::NODE_HEX, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Hex_BecomesMeleeTargetAfter_Action>::encoding)
	},
	EdgeType{
			 Graph::ElementType::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION,
			 "BecomesShootTargetAfter", {Graph::ElementType::NODE_HEX, Graph::ElementType::NODE_ACTION},
			 EncodedSize(EncodingTraits<Graph::EdgeAttributes::Hex_BecomesShootTargetAfter_Action>::encoding)
	}
};

static_assert(NODE_TYPES.size() + EDGE_TYPES.size() == static_cast<int>(Graph::ElementType::_count));

inline constexpr std::array ACTIVE_ACTION_EXCLUSIVE_EDGE_TYPES{
	Graph::ElementType::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION,
	Graph::ElementType::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION,
};

}
