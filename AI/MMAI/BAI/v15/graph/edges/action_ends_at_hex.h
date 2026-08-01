/*
 * action_ends_at_hex.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "BAI/v15/graph/edges/base.h"
#include "BAI/v15/graph/nodes/action.h"
#include "BAI/v15/graph/nodes/hex.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

namespace detail
{
	using Action_EndsAt_Hex_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::Action_EndsAt_Hex>;
	using Action_EndsAt_Hex_Base = Base<Nodes::Action, Nodes::Hex, Action_EndsAt_Hex_Traits>;
}

class Action_EndsAt_Hex : public detail::Action_EndsAt_Hex_Base
{
public:
	static std::shared_ptr<const Action_EndsAt_Hex>
	Create(const std::shared_ptr<const Nodes::Action> & srcNode, const std::shared_ptr<const Nodes::Hex> & dstNode, int isRear)
	{
		return std::make_shared<const Action_EndsAt_Hex>(srcNode, dstNode, isRear);
	};

	Action_EndsAt_Hex(const std::shared_ptr<const Nodes::Action> & srcNode, const std::shared_ptr<const Nodes::Hex> & dstNode, int isRear)
		: detail::Action_EndsAt_Hex_Base(srcNode, dstNode), isRear(isRear)
	{
		setattr(A::IS_REAR, isRear);
		static_assert(static_cast<size_t>(A::_count) == 1, "whistleblower in case attributes change");
	}

	const bool isRear;
};
}
