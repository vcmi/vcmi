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
#include "../../lib/AI_Base.h"
#include "../../lib/battle/ReachabilityInfo.h"
#include "PossibleSpellcast.h"
#include "PotentialTargets.h"

VCMI_LIB_NAMESPACE_BEGIN

class CSpell;

VCMI_LIB_NAMESPACE_END

class EnemyInfo;

/*
struct CurrentOffensivePotential
{
	std::map<const CStack *, PotentialTargets> ourAttacks;
	std::map<const CStack *, PotentialTargets> enemyAttacks;

	CurrentOffensivePotential(ui8 side)
	{
		for(auto stack : cbc->battleGetStacks())
		{
			if(stack->side == side)
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
*/ // These lines may be usefull but they are't used in the code.

class CBattleAI : public CBattleGameInterface
{
	int side;
	std::shared_ptr<CBattleCallback> cb;
	std::shared_ptr<Environment> env;

	//Previous setting of cb
	bool wasWaitingForRealize, wasUnlockingGs;

public:
	CBattleAI();
	~CBattleAI();

	void initBattleInterface(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB) override;
	void attemptCastingSpell();

	void evaluateCreatureSpellcast(const CStack * stack, PossibleSpellcast & ps); //for offensive damaging spells only

	BattleAction activeStack(const CStack * stack) override; //called when it's turn of that stack

	boost::optional<BattleAction> considerFleeingOrSurrendering();

	void print(const std::string &text) const;
	BattleAction useCatapult(const CStack *stack);
	void battleStart(const CCreatureSet * army1, const CCreatureSet * army2, int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2, bool Side) override;
	//void actionFinished(const BattleAction &action) override;//occurs AFTER every action taken by any stack or by the hero
	//void actionStarted(const BattleAction &action) override;//occurs BEFORE every action taken by any stack or by the hero
	//void battleAttack(const BattleAttack *ba) override; //called when stack is performing attack
	//void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) override; //called when stack receives damage (after battleAttack())
	//void battleEnd(const BattleResult *br) override;
	//void battleResultsApplied() override; //called when all effects of last battle are applied
	//void battleNewRoundFirst(int round) override; //called at the beginning of each turn before changes are applied;
	//void battleNewRound(int round) override; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	//void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance) override;
	//void battleSpellCast(const BattleSpellCast *sc) override;
	//void battleStacksEffectsSet(const SetStackEffect & sse) override;//called when a specific effect is set to stacks
	//void battleTriggerEffect(const BattleTriggerEffect & bte) override;
	//void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) override; //called by engine when battle starts; side=0 - left, side=1 - right
	//void battleCatapultAttacked(const CatapultAttack & ca) override; //called when catapult makes an attack

private:
	BattleAction goTowardsNearest(const CStack * stack, std::vector<BattleHex> hexes) const;
	std::vector<BattleHex> getBrokenWallMoatHexes() const;
};
