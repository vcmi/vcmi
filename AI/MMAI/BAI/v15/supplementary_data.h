/*
 * supplementary_data.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BAI/v15/attack_log.h"
#include "BAI/v15/graph/graph.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15
{
namespace S15 = Schema::V15;
using Side = Schema::Side;

// match sides for convenience when determining winner (see `victory`)
static_assert(EI(S15::CombatResult::LEFT_WINS) == EI(Side::LEFT));
static_assert(EI(S15::CombatResult::RIGHT_WINS) == EI(Side::RIGHT));

class SupplementaryData : public S15::ISupplementaryData
{
public:
	SupplementaryData() = delete;

	// Called on activeStack (complete battlefield info)
	SupplementaryData(
		const std::string & colorname,
		Side side,
		const std::shared_ptr<Graph::Graph> & G,
		const std::vector<AttackLog> & attackLogs,
		S15::CombatResult result
	)
		: colorname(colorname), side(side), G(G), attackLogs(attackLogs), ended(result != S15::CombatResult::NONE), victory(EI(result) == EI(side)) {};

	// impl ISupplementaryData
	Type getType() const override
	{
		return type;
	};

	const S15::Graph::IGraph * getGraph() const override
	{
		return G.get();
	}

	S15::AttackLogs getAttackLogs() const override;

	Side getSide() const
	{
		return side;
	}
	std::string getColor() const
	{
		return colorname;
	}
	bool getIsBattleEnded() const
	{
		return ended;
	}
	bool getIsVictorious() const
	{
		return victory;
	}

	std::string getAnsiRender() const override
	{
		return ansiRender;
	}

	const std::string colorname;
	const Side side;
	const std::shared_ptr<Graph::Graph> G;
	const std::vector<AttackLog> & attackLogs;
	const bool ended = false;
	const bool victory = false;

	// Optionally modified (during activeStack if action was RENDER)
	Type type = Type::REGULAR;
	std::string ansiRender;
};
}
