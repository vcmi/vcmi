/*
 * unit_is_shot_by_action.h, part of VCMI engine
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
#include "BAI/v15/graph/nodes/unit.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

namespace detail
{
	using Unit_IsShotBy_Action_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::Unit_IsShotBy_Action>;
	using Unit_IsShotBy_Action_Base = Base<Nodes::Unit, Nodes::Action, Unit_IsShotBy_Action_Traits>;
}

class Unit_IsShotBy_Action : public detail::Unit_IsShotBy_Action_Base
{
public:
	static std::shared_ptr<const Unit_IsShotBy_Action>
	Create(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Action> & dstNode, bool isPrimaryTarget)
	{
		return std::make_shared<const Unit_IsShotBy_Action>(srcNode, dstNode, isPrimaryTarget);
	};

	Unit_IsShotBy_Action(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Action> & dstNode, bool isPrimaryTarget)
		: detail::Unit_IsShotBy_Action_Base(srcNode, dstNode), isPrimaryTarget(isPrimaryTarget)
	{
		setattr(A::IS_PRIMARY_TARGET, isPrimaryTarget);
		static_assert(static_cast<size_t>(A::_count) == 1, "whistleblower in case attributes change");
	}

	const bool isPrimaryTarget;
};
}
