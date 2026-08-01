/*
 * hex_adjacent_hex.h, part of VCMI engine
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
#include "BAI/v15/graph/nodes/hex.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

namespace detail
{
	using Hex_Adjacent_Hex_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::Hex_Adjacent_Hex>;
	using Hex_Adjacent_Hex_Base = Base<Nodes::Hex, Nodes::Hex, Hex_Adjacent_Hex_Traits>;
}

class Hex_Adjacent_Hex : public detail::Hex_Adjacent_Hex_Base
{
public:
	static std::shared_ptr<const Hex_Adjacent_Hex>
	Create(const std::shared_ptr<const Nodes::Hex> & srcNode, const std::shared_ptr<const Nodes::Hex> & dstNode, int direction)
	{
		return std::make_shared<const Hex_Adjacent_Hex>(srcNode, dstNode, direction);
	};

	Hex_Adjacent_Hex(const std::shared_ptr<const Nodes::Hex> & srcNode, const std::shared_ptr<const Nodes::Hex> & dstNode, int direction)
		: detail::Hex_Adjacent_Hex_Base(srcNode, dstNode)
	{
		setattr(A::DIRECTION, direction);
		static_assert(static_cast<size_t>(A::_count) == 1, "whistleblower in case attributes change");
	}
};
}
