/*
 * unit_acts_before_unit.h, part of VCMI engine
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
#include "BAI/v15/graph/nodes/unit.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

namespace detail
{
	using Unit_ActsBefore_Unit_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::Unit_ActsBefore_Unit>;
	using Unit_ActsBefore_Unit_Base = Base<Nodes::Unit, Nodes::Unit, Unit_ActsBefore_Unit_Traits>;
}

class Unit_ActsBefore_Unit : public detail::Unit_ActsBefore_Unit_Base
{
public:
	static std::shared_ptr<const Unit_ActsBefore_Unit>
	Create(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Unit> & dstNode, const int times)
	{
		return std::make_shared<const Unit_ActsBefore_Unit>(srcNode, dstNode, times);
	};

	Unit_ActsBefore_Unit(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Unit> & dstNode, const int times)
		: detail::Unit_ActsBefore_Unit_Base(srcNode, dstNode), times(times)
	{
		setattr(A::TIMES, times);
		static_assert(static_cast<size_t>(A::_count) == 1, "whistleblower in case attributes change");
	}

	const int times;
};
}
