/*
 * unit_becomes_shoot_threat_after_action.h, part of VCMI engine
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
#include "BAI/v15/graph/util.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

namespace detail
{
	using Unit_BecomesShootThreatAfter_Action_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::Unit_BecomesShootThreatAfter_Action>;
	using Unit_BecomesShootThreatAfter_Action_Base = Base<Nodes::Unit, Nodes::Action, Unit_BecomesShootThreatAfter_Action_Traits>;
}

class Unit_BecomesShootThreatAfter_Action : public detail::Unit_BecomesShootThreatAfter_Action_Base
{
public:
	static std::shared_ptr<const Unit_BecomesShootThreatAfter_Action>
	Create(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Action> & dstNode, float mult)
	{
		return std::make_shared<const Unit_BecomesShootThreatAfter_Action>(srcNode, dstNode, mult);
	};

	Unit_BecomesShootThreatAfter_Action(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Action> & dstNode, float mult)
		: detail::Unit_BecomesShootThreatAfter_Action_Base(srcNode, dstNode), mult(mult)
	{
		setattr(A::DMG_MULT, permille(mult, 1));
		static_assert(static_cast<size_t>(A::_count) == 1, "whistleblower in case attributes change");
	}

	const float mult;
};
}
