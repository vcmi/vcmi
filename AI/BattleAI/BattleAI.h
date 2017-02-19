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
#include "PotentialTargets.h"

class CSpell;
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
			if(stack->attackerOwned == !side)
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

struct PossibleSpellcast
{
	const CSpell *spell;
	BattleHex dest;
	si32 value;
};

class CBattleAI : public CBattleGameInterface
{
	int side;
	std::shared_ptr<CBattleCallback> cb;

	//Previous setting of cb
	bool wasWaitingForRealize, wasUnlockingGs;

public:
	CBattleAI(void);
	~CBattleAI(void);

	void init(std::shared_ptr<CBattleCallback> CB) override;
	void attemptCastingSpell();

	BattleAction activeStack(const CStack * stack) override; //called when it's turn of that stack
	BattleAction goTowards(const CStack * stack, BattleHex hex );

	boost::optional<BattleAction> considerFleeingOrSurrendering();

	std::vector<BattleHex> getTargetsToConsider(const CSpell *spell, const ISpellCaster * caster) const;
	static int distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances& dists, BattleHex *chosenHex = nullptr);
	static bool isCloser(const EnemyInfo & ei1, const EnemyInfo & ei2, const ReachabilityInfo::TDistances & dists);

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
	//void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom) override; //called when stacks are healed / resurrected first element of pair - stack id, second - healed hp
	//void battleNewStackAppeared(const CStack * stack) override; //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	//void battleObstaclesRemoved(const std::set<si32> & removedObstacles) override; //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	//void battleCatapultAttacked(const CatapultAttack & ca) override; //called when catapult makes an attack
	//void battleStacksRemoved(const BattleStacksRemoved & bsr) override; //called when certain stack is completely removed from battlefield
};
