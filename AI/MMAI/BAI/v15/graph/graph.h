// =============================================================================
// Copyright 2024 Simeon Manolov <s.manolloff@gmail.com>.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// =============================================================================

#pragma once

#include "StdInc.h" // IWYU pragma: keep

#include "BAI/v15/graph/edges/hex_becomes_shoot_target_after_action.h"
#include "BAI/v15/graph/edges/unit_becomes_shoot_target_after_action.h"
#include "BAI/v15/graph/edges/unit_is_meleed_by_action.h"
#include "BAI/v15/graph/edges/unit_is_shot_by_action.h"
#include "battle/AccessibilityInfo.h"
#include "battle/CPlayerBattleCallback.h"

#include "BAI/v15/enum_flags.h"
#include "BAI/v15/graph/edge_store.h"
#include "BAI/v15/graph/edges/action_ends_at_hex.h"
#include "BAI/v15/graph/edges/generic.h"
#include "BAI/v15/graph/edges/hex_adjacent_hex.h"
#include "BAI/v15/graph/edges/hex_is_end_of_action.h"
#include "BAI/v15/graph/edges/unit_acts_before_unit.h"
#include "BAI/v15/graph/edges/unit_becomes_shoot_threat_after_action.h"
#include "BAI/v15/graph/edges/unit_melee_dmg_unit.h"
#include "BAI/v15/graph/edges/unit_shoot_dmg_unit.h"
#include "BAI/v15/graph/node_store.h"
#include "BAI/v15/graph/nodes/action.h"
#include "BAI/v15/graph/nodes/global.h"
#include "BAI/v15/graph/nodes/hex.h"
#include "BAI/v15/graph/nodes/player.h"
#include "BAI/v15/graph/nodes/unit.h"

#include "schema/v15/graph.h"
#include <tuple>

namespace MMAI::BAI::V15::Graph
{

namespace detail
{
	// Compile-time check for mistyped entries in EDGE_TYPES
	constexpr const S15::EdgeType & GetEdgeType(S15::Graph::ElementType type)
	{
		for(const auto & edge_type : S15::EDGE_TYPES)
		{
			if(std::get<0>(edge_type) == type)
			{
				return edge_type;
			}
		}

		throw std::out_of_range("Unknown edge type");
	}

	using TNodeStores = std::tuple<NodeStore<Nodes::Global>, NodeStore<Nodes::Player>, NodeStore<Nodes::Unit>, NodeStore<Nodes::Hex>, NodeStore<Nodes::Action>>;

	using TEdgeStores = std::tuple<
		EdgeStore<Edges::Global_To_Player>,
		EdgeStore<Edges::Player_To_Global>,
		EdgeStore<Edges::Global_To_Unit>,
		EdgeStore<Edges::Unit_To_Global>,
		EdgeStore<Edges::Global_To_Hex>,
		EdgeStore<Edges::Hex_To_Global>,
		EdgeStore<Edges::Global_To_Action>,
		EdgeStore<Edges::Player_Owns_Unit>,
		EdgeStore<Edges::Unit_OwnedBy_Player>,
		EdgeStore<Edges::Hex_Adjacent_Hex>,
		EdgeStore<Edges::Unit_ActsBefore_Unit>,
		EdgeStore<Edges::Unit_MeleeDmg_Unit>,
		EdgeStore<Edges::Unit_ShootDmg_Unit>,
		EdgeStore<Edges::Unit_Blocks_Unit>,
		EdgeStore<Edges::Unit_Occupies_Hex>,
		EdgeStore<Edges::Hex_OccupiedBy_Unit>,
		EdgeStore<Edges::Action_By_Unit>,
		EdgeStore<Edges::Unit_Has_Action>,
		EdgeStore<Edges::Action_EndsAt_Hex>,
		EdgeStore<Edges::Hex_IsEndOf_Action>,
		EdgeStore<Edges::Action_Blocks_Unit>,
		EdgeStore<Edges::Unit_BlockedBy_Action>,
		EdgeStore<Edges::Unit_BecomesMeleeThreatAfter_Action>,
		EdgeStore<Edges::Unit_BecomesShootThreatAfter_Action>,
		EdgeStore<Edges::Unit_IsMeleedBy_Action>,
		EdgeStore<Edges::Unit_IsShotBy_Action>,
		EdgeStore<Edges::Unit_BecomesMeleeTargetAfter_Action>,
		EdgeStore<Edges::Unit_BecomesShootTargetAfter_Action>,
		EdgeStore<Edges::Hex_BecomesMeleeTargetAfter_Action>,
		EdgeStore<Edges::Hex_BecomesShootTargetAfter_Action>>;

	static_assert(EU(S15::Graph::ElementType::_count) == std::tuple_size<TNodeStores>() + std::tuple_size<TEdgeStores>());

	template<typename T, typename Tuple>
	struct tuple_contains;

	template<typename T, typename... Ts>
	struct tuple_contains<T, std::tuple<Ts...>> : std::bool_constant<(std::same_as<T, Ts> || ...)>
	{
	};

	template<typename T>
	concept is_stored_node = tuple_contains<NodeStore<std::remove_cvref_t<T>>, TNodeStores>::value;

	template<typename T>
	concept is_stored_edge = tuple_contains<EdgeStore<std::remove_cvref_t<T>>, TEdgeStores>::value;

	template<typename T>
	concept is_stored_element = is_stored_node<T> || is_stored_edge<T>;

	template<typename EdgeType, typename NodeType>
	concept is_edge_src = std::is_same_v<typename EdgeType::src_node_type, NodeType>;

	template<typename EdgeType, typename NodeType>
	concept is_edge_dst = std::is_same_v<typename EdgeType::dst_node_type, NodeType>;

	template<typename>
	inline constexpr bool always_false = false;

	struct PrecalculatedNearbyPositions
	{
		const std::vector<BattleHex> &
		get(const BattleHex & bhex, BattleSide attackerSide, BattleSide defenderSide, bool isAttackerWide, bool isDefenderWide) const
		{
			return ary.at(static_cast<int>(attackerSide)).at(static_cast<int>(defenderSide))[isAttackerWide][isDefenderWide].at(bhex.toInt());
		}

		// dims: [attackerSide, defenderSide, isAttackerWide, isDefenderWide, bhex]
		std::array<std::array<std::array<std::array<std::array<std::vector<BattleHex>, GameConstants::BFIELD_SIZE>, 2>, 2>, 2>, 2> ary;
	};
}

namespace S15 = Schema::V15;

class Graph : public S15::Graph::IGraph
{
	using ET = S15::Graph::ElementType;

public:
	explicit Graph(const CPlayerBattleCallback & battle);

	Graph(const Graph &) = delete;
	Graph & operator=(const Graph &) = delete;
	Graph(Graph &&) = delete;
	Graph & operator=(Graph &&) = delete;

	// XXX: pass-by-value + move is preferred to pass-by-reference
	//      => must accept non-const std::shared ptr
	template<typename T>
	requires detail::is_stored_element<T>
	void add(std::shared_ptr<const T> elem)
	{
		// std::cout << "DEBUG: Add: " << elem->name() << " " << elem.get() << "\n";
		assert(elem);
		getMutableStore<T>().add(std::move(elem));
	}

	template<typename T>
	std::shared_ptr<const T> getById(std::size_t ind, bool strict = true) const
	{
		return getStore<T>().getById(ind, strict);
	}

	template<typename T>
	requires detail::is_stored_element<T>
	std::shared_ptr<const T> getByIdentity(const std::shared_ptr<const T> & elem, bool strict = true) const
	{
		assert(elem);
		return getStore<T>().getByIdentity(elem, strict);
	}

	template<typename T, typename Key>
	requires(!std::is_same_v<typename T::extra_index_type, void> && std::is_same_v<typename T::extra_index_type::result_type, Key>)
	std::shared_ptr<const T> getByExtraIndex(const Key & key, bool strict = true) const
	{
		return getStore<T>().getByExtraIndex(key, strict);
	}

	template<typename EdgeType, typename SrcNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_src<EdgeType, SrcNodeType>
	std::shared_ptr<const EdgeType> getOneEdgeBySrc(const std::shared_ptr<const SrcNodeType> & src, bool strict = true) const
	{
		assert(src);
		return getStore<EdgeType>().getOneBySrc(src, strict);
	}

	template<typename EdgeType, typename SrcNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_src<EdgeType, SrcNodeType>
	auto getOneEdgeDstBySrc(const std::shared_ptr<const SrcNodeType> & src, bool strict = true) const
	{
		assert(src);
		return getStore<EdgeType>().getOneDstBySrc(src, strict);
	}

	template<typename EdgeType, typename SrcNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_src<EdgeType, SrcNodeType>
	auto getAllEdgesBySrc(const std::shared_ptr<const SrcNodeType> & src) const
	{
		assert(src);
		return getStore<EdgeType>().getAllBySrc(src);
	}

	template<typename EdgeType, typename SrcNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_src<EdgeType, SrcNodeType>
	auto getAllEdgesDstBySrc(const std::shared_ptr<const SrcNodeType> & src) const
	{
		assert(src);
		return getStore<EdgeType>().getAllDstBySrc(src);
	}

	template<typename EdgeType, typename DstNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_dst<EdgeType, DstNodeType>
	std::shared_ptr<const EdgeType> getOneEdgeByDst(const std::shared_ptr<const DstNodeType> & dst, bool strict = true) const
	{
		assert(dst);
		return getStore<EdgeType>().getOneByDst(dst, strict);
	}

	template<typename EdgeType, typename DstNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_dst<EdgeType, DstNodeType>
	auto getOneEdgeSrcByDst(const std::shared_ptr<const DstNodeType> & dst, bool strict = true) const
	{
		assert(dst);
		return getStore<EdgeType>().getOneSrcByDst(dst, strict);
	}

	template<typename EdgeType, typename DstNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_dst<EdgeType, DstNodeType>
	auto getAllEdgesByDst(const std::shared_ptr<const DstNodeType> & dst) const
	{
		assert(dst);
		return getStore<EdgeType>().getAllByDst(dst);
	}

	template<typename EdgeType, typename DstNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_dst<EdgeType, DstNodeType>
	auto getAllEdgesSrcByDst(const std::shared_ptr<const DstNodeType> & dst) const
	{
		assert(dst);
		return getStore<EdgeType>().getAllSrcByDst(dst);
	}

	template<typename EdgeType, typename SrcNodeType, typename DstNodeType>
	requires detail::is_stored_edge<EdgeType> && detail::is_edge_src<EdgeType, SrcNodeType> && detail::is_edge_dst<EdgeType, DstNodeType>
	auto getEdgeBySrcDst(const std::shared_ptr<const SrcNodeType> & src, const std::shared_ptr<const DstNodeType> & dst, bool strict = true) const
	{
		assert(src && dst);
		return getStore<EdgeType>().getBySrcDst(src, dst, strict);
	}

	template<typename T>
	requires detail::is_stored_element<T>
	const auto & getAll() const
	{
		return getStore<T>().entries();
	}

	template<typename T>
	requires detail::is_stored_element<T>
	auto size() const
	{
		return getStore<T>().size();
	}

	template<typename T>
	requires detail::is_stored_element<T>
	std::ptrdiff_t getId(const std::shared_ptr<const T> & elem) const
	{
		assert(elem);
		return getStore<T>().getId(elem);
	}

	template<typename T>
	requires detail::is_stored_node<T>
	const auto & getStore() const
	{
		using U = std::remove_cvref_t<T>;
		return std::get<NodeStore<U>>(nodeStores);
	}

	template<typename T>
	requires detail::is_stored_edge<T>
	const auto & getStore() const
	{
		using U = std::remove_cvref_t<T>;
		return std::get<EdgeStore<U>>(edgeStores);
	}

	void verify() const;

	std::vector<const S15::Graph::INode *> getNodes(ET t) const override;

	std::vector<const S15::Graph::IEdge *> getEdges(ET t) const override;

	int64_t getNodeIndex(const S15::Graph::INode * node) const override;
	int64_t getEdgeIndex(const S15::Graph::IEdge * edge) const override;

	const S15::Graph::INode * getNode(ET t, std::size_t ind) const override;

	std::vector<int64_t> getActiveActionIds() const override;

	EnumFlags<ET> getFlags() const;
	void setFlag(ET et);

	const AccessibilityInfo & getAccessibility() const;
	const FastBFS & getFastBFS() const;
	const detail::PrecalculatedNearbyPositions & getNearbyPositions() const;

private:
	EnumFlags<ET> flags;

	detail::TNodeStores nodeStores;
	detail::TEdgeStores edgeStores;

	const AccessibilityInfo accessibility;
	const FastBFS fastbfs;
	const detail::PrecalculatedNearbyPositions nearbyPositions;

	// identical to getStore(), but returned type is non-const
	template<typename T>
	requires detail::is_stored_node<T>
	auto & getMutableStore()
	{
		using U = std::remove_cvref_t<T>;
		return std::get<NodeStore<U>>(nodeStores);
	}

	template<typename T>
	requires detail::is_stored_edge<T>
	auto & getMutableStore()
	{
		using U = std::remove_cvref_t<T>;
		return std::get<EdgeStore<U>>(edgeStores);
	}

	//
	// Convenience for retrieving store by ElementType. Usage:
	//      withNodeStore(ET::Player, [](const auto& store) { ... });
	//
	template<typename F>
	decltype(auto) withNodeStore(ET t, F && f) const
	{
		switch(t)
		{
			case ET::NODE_GLOBAL:
				return std::forward<F>(f)(getStore<Nodes::Global>());
			case ET::NODE_PLAYER:
				return std::forward<F>(f)(getStore<Nodes::Player>());
			case ET::NODE_UNIT:
				return std::forward<F>(f)(getStore<Nodes::Unit>());
			case ET::NODE_HEX:
				return std::forward<F>(f)(getStore<Nodes::Hex>());
			case ET::NODE_ACTION:
				return std::forward<F>(f)(getStore<Nodes::Action>());
			default:
				throw std::runtime_error("Unexpected node element type: " + std::to_string(EU(t)));
		}
	}

	// XXX: these static assertions can be placed anywhere, but are here because
	//      the loop over all edge types is a convenient place to put them.
	//      The surrounding code provides context and it's harder to forget to
	//      add a check.
	template<typename F>
	decltype(auto) withEdgeStore(ET t, F && f) const
	{
		switch(t)
		{
			case ET::EDGE_GLOBAL_TO_PLAYER:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_PLAYER)).first == ET::NODE_GLOBAL);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_PLAYER)).second == ET::NODE_PLAYER);
				return std::forward<F>(f)(getStore<Edges::Global_To_Player>());
			case ET::EDGE_PLAYER_TO_GLOBAL:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_PLAYER_TO_GLOBAL)).first == ET::NODE_PLAYER);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_PLAYER_TO_GLOBAL)).second == ET::NODE_GLOBAL);
				return std::forward<F>(f)(getStore<Edges::Player_To_Global>());
			case ET::EDGE_GLOBAL_TO_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_UNIT)).first == ET::NODE_GLOBAL);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Global_To_Unit>());
			case ET::EDGE_UNIT_TO_GLOBAL:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_TO_GLOBAL)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_TO_GLOBAL)).second == ET::NODE_GLOBAL);
				return std::forward<F>(f)(getStore<Edges::Unit_To_Global>());
			case ET::EDGE_GLOBAL_TO_HEX:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_HEX)).first == ET::NODE_GLOBAL);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_HEX)).second == ET::NODE_HEX);
				return std::forward<F>(f)(getStore<Edges::Global_To_Hex>());
			case ET::EDGE_HEX_TO_GLOBAL:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_TO_GLOBAL)).first == ET::NODE_HEX);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_TO_GLOBAL)).second == ET::NODE_GLOBAL);
				return std::forward<F>(f)(getStore<Edges::Hex_To_Global>());
			case ET::EDGE_GLOBAL_TO_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_ACTION)).first == ET::NODE_GLOBAL);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_GLOBAL_TO_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Global_To_Action>());
			case ET::EDGE_PLAYER_OWNS_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_PLAYER_OWNS_UNIT)).first == ET::NODE_PLAYER);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_PLAYER_OWNS_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Player_Owns_Unit>());
			case ET::EDGE_UNIT_OWNED_BY_PLAYER:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_OWNED_BY_PLAYER)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_OWNED_BY_PLAYER)).second == ET::NODE_PLAYER);
				return std::forward<F>(f)(getStore<Edges::Unit_OwnedBy_Player>());
			case ET::EDGE_HEX_ADJACENT_HEX:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_ADJACENT_HEX)).first == ET::NODE_HEX);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_ADJACENT_HEX)).second == ET::NODE_HEX);
				return std::forward<F>(f)(getStore<Edges::Hex_Adjacent_Hex>());
			case ET::EDGE_UNIT_ACTS_BEFORE_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_ACTS_BEFORE_UNIT)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_ACTS_BEFORE_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Unit_ActsBefore_Unit>());
			case ET::EDGE_UNIT_MELEE_DMG_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_MELEE_DMG_UNIT)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_MELEE_DMG_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Unit_MeleeDmg_Unit>());
			case ET::EDGE_UNIT_SHOOT_DMG_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_SHOOT_DMG_UNIT)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_SHOOT_DMG_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Unit_ShootDmg_Unit>());
			case ET::EDGE_UNIT_BLOCKS_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BLOCKS_UNIT)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BLOCKS_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Unit_Blocks_Unit>());
			case ET::EDGE_UNIT_OCCUPIES_HEX:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_OCCUPIES_HEX)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_OCCUPIES_HEX)).second == ET::NODE_HEX);
				return std::forward<F>(f)(getStore<Edges::Unit_Occupies_Hex>());
			case ET::EDGE_HEX_OCCUPIED_BY_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_OCCUPIED_BY_UNIT)).first == ET::NODE_HEX);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_OCCUPIED_BY_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Hex_OccupiedBy_Unit>());
			case ET::EDGE_ACTION_BY_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_ACTION_BY_UNIT)).first == ET::NODE_ACTION);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_ACTION_BY_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Action_By_Unit>());
			case ET::EDGE_UNIT_HAS_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_HAS_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_HAS_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_Has_Action>());
			case ET::EDGE_ACTION_ENDS_AT_HEX:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_ACTION_ENDS_AT_HEX)).first == ET::NODE_ACTION);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_ACTION_ENDS_AT_HEX)).second == ET::NODE_HEX);
				return std::forward<F>(f)(getStore<Edges::Action_EndsAt_Hex>());
			case ET::EDGE_HEX_IS_END_OF_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_IS_END_OF_ACTION)).first == ET::NODE_HEX);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_IS_END_OF_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Hex_IsEndOf_Action>());
			case ET::EDGE_ACTION_BLOCKS_UNIT:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_ACTION_BLOCKS_UNIT)).first == ET::NODE_ACTION);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_ACTION_BLOCKS_UNIT)).second == ET::NODE_UNIT);
				return std::forward<F>(f)(getStore<Edges::Action_Blocks_Unit>());
			case ET::EDGE_UNIT_BLOCKED_BY_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BLOCKED_BY_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BLOCKED_BY_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_BlockedBy_Action>());
			case ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_MELEE_THREAT_AFTER_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_BecomesMeleeThreatAfter_Action>());
			case ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_SHOOT_THREAT_AFTER_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_BecomesShootThreatAfter_Action>());
			case ET::EDGE_UNIT_IS_MELEED_BY_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_IS_MELEED_BY_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_IS_MELEED_BY_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_IsMeleedBy_Action>());
			case ET::EDGE_UNIT_IS_SHOT_BY_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_IS_SHOT_BY_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_IS_SHOT_BY_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_IsShotBy_Action>());
			case ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_MELEE_TARGET_AFTER_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_BecomesMeleeTargetAfter_Action>());
			case ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION)).first == ET::NODE_UNIT);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_UNIT_BECOMES_SHOOT_TARGET_AFTER_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Unit_BecomesShootTargetAfter_Action>());
			case ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION)).first == ET::NODE_HEX);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_BECOMES_MELEE_TARGET_AFTER_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Hex_BecomesMeleeTargetAfter_Action>());
			case ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION:
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION)).first == ET::NODE_HEX);
				static_assert(std::get<2>(detail::GetEdgeType(ET::EDGE_HEX_BECOMES_SHOOT_TARGET_AFTER_ACTION)).second == ET::NODE_ACTION);
				return std::forward<F>(f)(getStore<Edges::Hex_BecomesShootTargetAfter_Action>());
			default:
				throw std::runtime_error("Unexpected edge element type: " + std::to_string(EU(t)));
		}
		static_assert(static_cast<int>(S15::Graph::ElementType::_count) == 35);
	}
};
}
