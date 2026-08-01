/*
 * hex.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "common.h" // IWYU pragma: keep

#include "BAI/v15/graph/nodes/base.h"
#include "battle/AccessibilityInfo.h"
#include "battle/BattleHex.h"
#include "battle/CObstacleInstance.h"
#include "schema/v15/constants.h"
#include "schema/v15/graph.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15::Graph::Nodes
{
namespace S15 = Schema::V15;

namespace detail
{
	using Hex_Traits = S15::EncodingTraits<S15::Graph::NodeAttributes::Hex>;
	using Hex_Base = Base<Hex_Traits>;
}

class Hex : public detail::Hex_Base
{
public:
	using HexActionHex = std::array<BattleHex, 12>;

	struct extra_index_type
	{
		using result_type = int16_t;
		result_type operator()(const std::shared_ptr<const Hex> & hex) const
		{
			return hex->bhex.toInt();
		}
	};

	struct Args
	{
		const BattleHex & bhex;
		const EAccessibility accessibility;
		const BattleSide side;
		const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles;
		const S15::WallHP wallHP;
		const bool isGateOpen;
		const bool isSiege;
	};

	static std::shared_ptr<const Hex> Create(const Args & args)
	{
		return std::make_shared<const Hex>(args);
	}

	static int CalcId(const BattleHex & bh);
	static std::pair<int, int> CalcXY(const BattleHex & bh);

	explicit Hex(const Args & args);

	std::string name() const override;

	// Disable guardflags as some of the IS_* attributes may remain unset
	int attr(Attribute a) const;
	void setattr(Attribute a, int value);

	const BattleHex bhex;
	const int id;

private:
	void setStateMask(EAccessibility accessibility, const std::vector<std::shared_ptr<const CObstacleInstance>> & obstacles, BattleSide side, bool isGateOpen);

	void setMoatFlags(const CObstacleInstance * obstacle, bool isGateOpen, BattleSide side);
};

}
