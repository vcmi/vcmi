/*
 * router.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "battle/AutocombatPreferences.h"
#include "battle/CPlayerBattleCallback.h"

#include "BAI/base.h"

namespace MMAI::BAI
{
class Router : public CBattleGameInterface
{
public:
	Router();
	~Router() override;

	/*
	 * Handled locally (not delegated)
	 */

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences prefs) override;

	/*
	 * Delegated to BAI
	 */

	void actionFinished(const BattleID & bid, const BattleAction & action) override;
	void actionStarted(const BattleID & bid, const BattleAction & action) override;
	void activeStack(const BattleID & bid, const CStack * stack) override; //called when it's turn of that stack
	void battleAttack(const BattleID & bid, const BattleAttack * ba) override;
	void battleCatapultAttacked(const BattleID & bid, const CatapultAttack & ca) override;
	void battleEnd(const BattleID & bid, const BattleResult * br, QueryID queryID) override;
	void battleGateStateChanged(const BattleID & bid, EGateState state) override;
	void battleLogMessage(const BattleID & bid, const std::vector<MetaString> & lines) override;
	void battleNewRound(const BattleID & bid) override;
	void battleNewRoundFirst(const BattleID & bid) override;
	void battleObstaclesChanged(const BattleID & bid, const std::vector<ObstacleChanges> & obstacles) override;
	void battleSpellCast(const BattleID & bid, const BattleSpellCast * sc) override;
	void battleStackMoved(const BattleID & bid, const CStack * stack, const BattleHexArray & dest, int distance, bool teleport) override;
	void battleStacksAttacked(const BattleID & bid, const std::vector<BattleStackAttacked> & bsa, bool ranged) override;
	void battleStacksEffectsSet(const BattleID & bid, const SetStackEffect & sse) override;
	void battleStart(
		const BattleID & bid,
		const CCreatureSet * army1,
		const CCreatureSet * army2,
		int3 tile,
		const CGHeroInstance * hero1,
		const CGHeroInstance * hero2,
		BattleSide side,
		bool replayAllowed
	) override;
	void battleTriggerEffect(const BattleID & bid, const BattleTriggerEffect & bte) override;
	void battleUnitsChanged(const BattleID & bid, const std::vector<UnitChanges> & changes) override;
	void yourTacticPhase(const BattleID & bid, int distance) override;

private:
	std::shared_ptr<Environment> env;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<CBattleGameInterface> bai; // calls will be delegated to this object

	bool wasWaitingForRealize = false;
	AutocombatPreferences autocombatPreferences;
	std::string addrstr = "?";
	std::string colorname = "?";

	void error(const std::string & text) const;
	void warn(const std::string & text) const;
	void info(const std::string & text) const;
	void debug(const std::string & text) const;
	void trace(const std::string & text) const;
	void log(ELogLevel::ELogLevel level, const std::string & text) const;
};
}
