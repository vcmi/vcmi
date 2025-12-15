/*
 * BAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BAI/base.h"
#include "BAI/v13/action.h"
#include "BAI/v13/state.h"

namespace MMAI::BAI::V13
{
class BAI : public Base
{
public:
	using Base::Base;

	// Bring thes template functions into the derived class's scope
	using Base::_log;
	using Base::debug;
	using Base::error;
	using Base::info;
	using Base::log;
	using Base::trace;
	using Base::warn;

	void activeStack(const BattleID & bid, const CStack * stack) override;
	void actionStarted(const BattleID & bid, const BattleAction & action) override;
	void actionFinished(const BattleID & bid, const BattleAction & action) override;
	void yourTacticPhase(const BattleID & bid, int distance) override;

	void battleStacksAttacked(
		const BattleID & bid,
		const std::vector<BattleStackAttacked> & bsa,
		bool ranged
	) override; //called when stack receives damage (after battleAttack())
	void battleTriggerEffect(const BattleID & bid, const BattleTriggerEffect & bte) override;
	void battleEnd(const BattleID & bid, const BattleResult * br, QueryID queryID) override;
	void battleStart(
		const BattleID & bid,
		const CCreatureSet * army1,
		const CCreatureSet * army2,
		int3 tile,
		const CGHeroInstance * hero1,
		const CGHeroInstance * hero2,
		BattleSide side,
		bool replayAllowed
	) override; //called by engine when battle starts; side=0 - left, side=1 - right

	Schema::Action getNonRenderAction() override;

	// Subsequent versions may override this with subclasses of State
	virtual std::unique_ptr<State> initState(const CPlayerBattleCallback * battle);
	std::unique_ptr<State> state = nullptr;

	// consecutive invalid actions counter
	int errcounter = 0;

	int getActionTotalMs;
	int getActionTotalCalls;

	bool resetting = false;
	std::vector<Schema::Action> allactions; // DEBUG ONLY
	std::shared_ptr<CPlayerBattleCallback> battle = nullptr;

	std::string renderANSI() const;
	std::string debugInfo(Action * action, const CStack * astack, const BattleHex * nbh); // DEBUG ONLY
	void handleUnexpectedAction(const CStack * acstack, std::unique_ptr<Hex> & hex, Action * action);
	std::shared_ptr<BattleAction> buildBattleAction();
	std::shared_ptr<BattleAction> maybeBuildAutoAction(const CStack * stack, const BattleID & bid) const;
	bool maybeCastSpell(const CStack * stack, const BattleID & bid);

	std::optional<BattleAction> maybeFleeOrSurrender(const BattleID & bid);
};
}
