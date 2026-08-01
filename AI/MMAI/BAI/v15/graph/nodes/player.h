/*
 * player.h, part of VCMI engine
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
#include "battle/BattleSide.h"
#include "schema/v15/constants.h"
#include "schema/v15/graph.h"

namespace MMAI::BAI::V15::Graph::Nodes
{
namespace S15 = Schema::V15;

namespace detail
{
	using Player_Traits = S15::EncodingTraits<S15::Graph::NodeAttributes::Player>;
	using Player_Base = Base<Player_Traits>;
}

class Player : public detail::Player_Base
{
public:
	struct extra_index_type
	{
		using result_type = BattleSide;
		result_type operator()(const std::shared_ptr<const Player> & player) const
		{
			return player->side;
		}
	};

	struct Args
	{
		const BattleSide side;
		const bool isActive;
		const int globalValueStart;
		const int globalValuePrevRound;
		const int globalHpPrevRound;
		const int value;
		const int hp;
		const int dmgDealt;
		const int dmgReceived;
		const int valueKilled;
		const int valueLost;
	};

	static std::shared_ptr<const Player> Create(const Args & args)
	{
		return std::make_shared<const Player>(args);
	}

	explicit Player(const Args & args);

	std::string name() const override
	{
		std::stringstream ss;
		ss << detail::Player_Base::name() << "(" << static_cast<int>(side) << ")";
		return ss.str();
	}

	BattleSide side;
};

}
