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

#include "BAI/v13/attack_log.h"
#include "BAI/v13/battlefield.h"
#include "BAI/v13/global_stats.h"
#include "BAI/v13/player_stats.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13
{
using Side = Schema::Side;
using ErrorCode = Schema::V13::ErrorCode;

// match sides for convenience when determining winner (see `victory`)
static_assert(EI(CombatResult::LEFT_WINS) == EI(Side::LEFT));
static_assert(EI(CombatResult::RIGHT_WINS) == EI(Side::RIGHT));

class SupplementaryData : public Schema::V13::ISupplementaryData
{
public:
	SupplementaryData() = delete;

	// Called on activeStack (complete battlefield info)
	SupplementaryData(
		const std::string & colorname_,
		Side side_,
		const GlobalStats * gstats_,
		const PlayerStats * lpstats_,
		const PlayerStats * rpstats_,
		const Battlefield * battlefield_,
		const std::vector<std::shared_ptr<AttackLog>> & attackLogs_,
		const std::vector<std::tuple<Schema::Action, std::shared_ptr<Schema::ActionMask>, std::shared_ptr<Schema::BattlefieldState>>> & transitions_,
		CombatResult result
	)
		: colorname(colorname_)
		, side(side_)
		, battlefield(battlefield_)
		, gstats(gstats_)
		, lpstats(lpstats_)
		, rpstats(rpstats_)
		, attackLogs(attackLogs_)
		, transitions(transitions_)
		, ended(result != CombatResult::NONE)
		, victory(EI(result) == EI(side)) {};

	// impl ISupplementaryData
	Type getType() const override
	{
		return type;
	};
	Side getSide() const override
	{
		return side;
	};
	std::string getColor() const override
	{
		return colorname;
	};
	ErrorCode getErrorCode() const override
	{
		return errcode;
	};

	bool getIsBattleEnded() const override
	{
		return ended;
	};
	bool getIsVictorious() const override
	{
		return victory;
	};

	Schema::V13::Stacks getStacks() const override;
	Schema::V13::Hexes getHexes() const override;
	Schema::V13::AllLinks getAllLinks() const override;
	Schema::V13::AttackLogs getAttackLogs() const override;
	Schema::V13::StateTransitions getStateTransitions() const override;
	const Schema::V13::IGlobalStats * getGlobalStats() const override
	{
		return gstats;
	}
	const Schema::V13::IPlayerStats * getLeftPlayerStats() const override
	{
		return lpstats;
	}
	const Schema::V13::IPlayerStats * getRightPlayerStats() const override
	{
		return rpstats;
	}
	std::string getAnsiRender() const override
	{
		return ansiRender;
	}

	const std::string colorname;
	const Side side;
	const Battlefield * const battlefield;
	const GlobalStats * const gstats;
	const PlayerStats * const lpstats;
	const PlayerStats * const rpstats;
	const std::vector<std::shared_ptr<AttackLog>> attackLogs;
	const std::vector<std::tuple<Schema::Action, std::shared_ptr<Schema::ActionMask>, std::shared_ptr<Schema::BattlefieldState>>> transitions;
	const bool ended = false;
	const bool victory = false;

	// Optionally modified (during activeStack if action was invalid)
	ErrorCode errcode = ErrorCode::OK;

	// Optionally modified (during activeStack if action was RENDER)
	Type type = Type::REGULAR;
	std::string ansiRender;
};
}
