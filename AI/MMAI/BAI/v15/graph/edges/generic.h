/*
 * generic.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "common.h" // IWYU pragma: keep

#include "BAI/v15/graph/edges/base.h"
#include "BAI/v15/graph/nodes/action.h"
#include "BAI/v15/graph/nodes/global.h"
#include "BAI/v15/graph/nodes/hex.h"
#include "BAI/v15/graph/nodes/player.h"
#include "BAI/v15/graph/nodes/unit.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

/*
 * The macro is useful for generic edges which have no attributes.
 *
 * GENERIC_EDGE_ELEMENT(NodeA, Edge, NodeB) expands to:
 *
 *     using NodeA_Adjacent_NodeB_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::NodeA_Edge_NodeB>;
 *     using NodeA_Edge_NodeB_Base = Base<Nodes::NodeA, Nodes::NodeB, NodeA_Edge_NodeB_Traits>;
 *     class NodeA_Edge_NodeA : public S15::EncodingTraits<
 *         NodeA,
 *         NodeB,
 *         S15::EncodingTraits<S15::EdgeEncoding_NodeA_Edge_NodeB>
 *     > {};
 */
#define GENERIC_EDGE_ELEMENT(NodeA, Edge, NodeB)                                                                             \
	namespace detail                                                                                                         \
	{                                                                                                                        \
		using NodeA##_##Edge##_##NodeB##_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::NodeA##_##Edge##_##NodeB>; \
		using NodeA##_##Edge##_##NodeB##_Base = Base<Nodes::NodeA, Nodes::NodeB, NodeA##_##Edge##_##NodeB##_Traits>;         \
	}                                                                                                                        \
	class NodeA##_##Edge##_##NodeB : public detail::NodeA##_##Edge##_##NodeB##_Base                                          \
	{                                                                                                                        \
	public:                                                                                                                  \
		using detail::NodeA##_##Edge##_##NodeB##_Base::NodeA##_##Edge##_##NodeB##_Base;                                      \
		static_assert(EU(A::_count) == 0, "generic edges cannot have attributes");                                           \
		static std::shared_ptr<const NodeA##_##Edge##_##NodeB>                                                               \
		Create(const std::shared_ptr<const Nodes::NodeA> & srcNode, const std::shared_ptr<const Nodes::NodeB> & dstNode)     \
		{                                                                                                                    \
			return std::make_shared<const NodeA##_##Edge##_##NodeB>(srcNode, dstNode);                                       \
		}                                                                                                                    \
	}

GENERIC_EDGE_ELEMENT(Global, To, Player);
GENERIC_EDGE_ELEMENT(Player, To, Global);
GENERIC_EDGE_ELEMENT(Global, To, Unit);
GENERIC_EDGE_ELEMENT(Unit, To, Global);
GENERIC_EDGE_ELEMENT(Global, To, Hex);
GENERIC_EDGE_ELEMENT(Hex, To, Global);
GENERIC_EDGE_ELEMENT(Global, To, Action);

GENERIC_EDGE_ELEMENT(Player, Owns, Unit);
GENERIC_EDGE_ELEMENT(Unit, OwnedBy, Player);

GENERIC_EDGE_ELEMENT(Unit, Blocks, Unit);
GENERIC_EDGE_ELEMENT(Unit, Occupies, Hex);
GENERIC_EDGE_ELEMENT(Hex, OccupiedBy, Unit);

GENERIC_EDGE_ELEMENT(Action, By, Unit);
GENERIC_EDGE_ELEMENT(Unit, Has, Action);
GENERIC_EDGE_ELEMENT(Action, Blocks, Unit);
GENERIC_EDGE_ELEMENT(Unit, BlockedBy, Action);
GENERIC_EDGE_ELEMENT(Unit, BecomesMeleeThreatAfter, Action);
GENERIC_EDGE_ELEMENT(Unit, BecomesMeleeTargetAfter, Action);
GENERIC_EDGE_ELEMENT(Hex, BecomesMeleeTargetAfter, Action);

static_assert(static_cast<int>(S15::Graph::ElementType::_count) == 35);

}
