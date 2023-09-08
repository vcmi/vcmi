/*
 * StupidAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/battle/BattleHex.h"
#include "../../lib/battle/ReachabilityInfo.h"
#include "../../lib/CGameInterface.h"

class EnemyInfo;

class CStupidAI : public CBattleGameInterface
{
	int side;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;

	bool wasWaitingForRealize;
	bool wasUnlockingGs;

	void print(const std::string &text) const;
public:
	CStupidAI();
	~CStupidAI();

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences) override;

	void actionFinished(const BattleID & battleID, const BattleAction &action) override;//occurs AFTER every action taken by any stack or by the hero
	void actionStarted(const BattleID & battleID, const BattleAction &action) override;//occurs BEFORE every action taken by any stack or by the hero
	void activeStack(const BattleID & battleID, const CStack * stack) override; //called when it's turn of that stack
	void yourTacticPhase(const BattleID & battleID, int distance) override;

	void battleAttack(const BattleID & battleID, const BattleAttack *ba) override; //called when stack is performing attack
	void battleStacksAttacked(const BattleID & battleID, const std::vector<BattleStackAttacked> & bsa, bool ranged) override; //called when stack receives damage (after battleAttack())
	void battleEnd(const BattleID & battleID, const BattleResult *br, QueryID queryID) override;
	//void battleResultsApplied() override; //called when all effects of last battle are applied
	void battleNewRoundFirst(const BattleID & battleID) override; //called at the beginning of each turn before changes are applied;
	void battleNewRound(const BattleID & battleID) override; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport) override;
	void battleSpellCast(const BattleID & battleID, const BattleSpellCast *sc) override;
	void battleStacksEffectsSet(const BattleID & battleID, const SetStackEffect & sse) override;//called when a specific effect is set to stacks
	//void battleTriggerEffect(const BattleTriggerEffect & bte) override;
	void battleStart(const BattleID & battleID, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side, bool replayAllowed) override; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleCatapultAttacked(const BattleID & battleID, const CatapultAttack & ca) override; //called when catapult makes an attack

private:
	BattleAction goTowards(const BattleID & battleID, const CStack * stack, std::vector<BattleHex> hexes) const;
};

