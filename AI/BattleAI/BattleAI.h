/*
 * BattleAI.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/battle/ReachabilityInfo.h"
#include "../../lib/callback/CGameInterface.h"
#include "PossibleSpellcast.h"
#include "PotentialTargets.h"
#include "TacticsHandler.h"

class CSpell;

/*
struct CurrentOffensivePotential
{
	std::map<const CStack *, PotentialTargets> ourAttacks;
	std::map<const CStack *, PotentialTargets> enemyAttacks;

	CurrentOffensivePotential(BattleSide side)
	{
		for(auto stack : cbc->battleGetStacks())
		{
			if(stack->unitSide() == side)
				ourAttacks[stack] = PotentialTargets(stack);
			else
				enemyAttacks[stack] = PotentialTargets(stack);
		}
	}

	int potentialValue()
	{
		int ourPotential = 0, enemyPotential = 0;
		for(auto &p : ourAttacks)
			ourPotential += p.second.bestAction().attackValue();

		for(auto &p : enemyAttacks)
			enemyPotential += p.second.bestAction().attackValue();

		return ourPotential - enemyPotential;
	}
};
*/ // These lines may be useful but they are't used in the code.

class CBattleAI : public CBattleGameInterface
{
	BattleSide side;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;

	//Previous setting of cb
	int movesSkippedByDefense;

	std::unique_ptr<TacticsHandler> tacticsHandler;

public:
	CBattleAI();
	~CBattleAI();

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB, AutocombatPreferences autocombatPreferences) override;

	void activeStack(const BattleID & battleID, const CStack * stack) override; //called when it's turn of that stack
	void yourTacticPhase(const BattleID & battleID, int distance) override;

	std::optional<BattleAction> considerFleeingOrSurrendering(const BattleID & battleID);

	void print(const std::string &text) const;
	BattleAction useCatapult(const BattleID & battleID, const CStack *stack);
	BattleAction useHealingTent(const BattleID & battleID, const CStack *stack);

	void battleStart(const BattleID & battleID, const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, BattleSide side, bool replayAllowed) override;
	void actionFinished(const BattleID & battleID, const BattleAction & action) override;
	AutocombatPreferences autobattlePreferences = AutocombatPreferences();
};
