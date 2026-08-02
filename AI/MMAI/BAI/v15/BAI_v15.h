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

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "BAI/logger.h"
#include "BAI/v15/state_v15.h"
#include "TacticsHandler.h"
#include "callback/CBattleGameInterface.h"

namespace MMAI::BAI::V15
{
class BAI : public CBattleGameInterface
{
	using ActionPtr = std::shared_ptr<const Graph::Nodes::Action>;

public:
	BAI(Schema::IModel * model,
		int version,
		const std::shared_ptr<Environment> & env,
		const std::shared_ptr<CBattleCallback> & cb,
		bool enableSpells,
		bool enableTactics);

	void activeStack(const BattleID & bid, const CStack * stack) override;
	void battleNewRound(const BattleID & bid) override;
	void yourTacticPhase(const BattleID & bid, int distance) override;
	void battleStackMoved(const BattleID & battleID, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport) override;

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

	Schema::Action getNonRenderAction();

	// Subsequent versions may override this with subclasses of State
	virtual std::unique_ptr<State> initState(const CPlayerBattleCallback * battle);
	std::unique_ptr<State> state = nullptr;
	ActionPtr lastAction = nullptr;

	Schema::IModel * model;
	const int version;
	const Logger logger;
	const std::shared_ptr<Environment> env;
	const std::shared_ptr<CBattleCallback> cb;

	bool enableSpells = false;
	bool enableTactics = false;

	// consecutive invalid actions counter
	int errcounter = 0;
	int roundcounter = 0;
	bool inFallback = false;

	int getActionTotalMs = 0;
	int getActionTotalCalls = 0;

	bool resetting = false;
	std::vector<Schema::Action> allactions; // DEBUG ONLY
	std::shared_ptr<CPlayerBattleCallback> battle = nullptr;

	std::shared_ptr<TacticsHandler> tacticsHandler;

	std::string renderANSI() const;
	std::shared_ptr<BattleAction> buildBattleAction(const ActionPtr & a, const CStack * acstack) const;
	std::shared_ptr<BattleAction> maybeBuildAutoAction(const CStack * stack, const BattleID & bid) const;
	bool maybeCastSpell(const CStack * stack, const BattleID & bid);
	void _activeStack(const BattleID & bid, const CStack * stack);
	bool guardVip(const BattleID & bid, const CStack * guard, const CStack * vip);
	void tacticMove(const BattleID & bid, const CStack * cstack, const BattleHex & bh);

	std::optional<BattleAction> maybeFleeOrSurrender(const BattleID & bid);
};
}
