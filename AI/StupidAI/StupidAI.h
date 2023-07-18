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

class EnemyInfo;

class CStupidAI : public CBattleGameInterface
{
	int side;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;

	void print(const std::string &text) const;
public:
	CStupidAI();
	~CStupidAI();

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void actionFinished(const BattleAction &action) override;//occurs AFTER every action taken by any stack or by the hero
	void actionStarted(const BattleAction &action) override;//occurs BEFORE every action taken by any stack or by the hero
	void activeStack(const CStack * stack) override; //called when it's turn of that stack

	void battleAttack(const BattleAttack *ba) override; //called when stack is performing attack
	void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa, bool ranged) override; //called when stack receives damage (after battleAttack())
	void battleEnd(const BattleResult *br, QueryID queryID) override;
	//void battleResultsApplied() override; //called when all effects of last battle are applied
	void battleNewRoundFirst(int round) override; //called at the beginning of each turn before changes are applied;
	void battleNewRound(int round) override; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance, bool teleport) override;
	void battleSpellCast(const BattleSpellCast *sc) override;
	void battleStacksEffectsSet(const SetStackEffect & sse) override;//called when a specific effect is set to stacks
	//void battleTriggerEffect(const BattleTriggerEffect & bte) override;
	void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) override; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleCatapultAttacked(const CatapultAttack & ca) override; //called when catapult makes an attack

private:
	BattleAction goTowards(const CStack * stack, std::vector<BattleHex> hexes) const;
};

