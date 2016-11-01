#pragma once

#include "../../lib/BattleHex.h"

class CStupidAI : public CBattleGameInterface
{
	int side;
	std::shared_ptr<CBattleCallback> cb;

	void print(const std::string &text) const;
public:
	CStupidAI(void);
	~CStupidAI(void);

	void init(std::shared_ptr<CBattleCallback> CB) override;
	void actionFinished(const BattleAction &action) override;//occurs AFTER every action taken by any stack or by the hero
	void actionStarted(const BattleAction &action) override;//occurs BEFORE every action taken by any stack or by the hero
	BattleAction activeStack(const CStack * stack) override; //called when it's turn of that stack

	void battleAttack(const BattleAttack *ba) override; //called when stack is performing attack
	void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) override; //called when stack receives damage (after battleAttack())
	void battleEnd(const BattleResult *br) override;
	//void battleResultsApplied() override; //called when all effects of last battle are applied
	void battleNewRoundFirst(int round) override; //called at the beginning of each turn before changes are applied;
	void battleNewRound(int round) override; //called at the beginning of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance) override;
	void battleSpellCast(const BattleSpellCast *sc) override;
	void battleStacksEffectsSet(const SetStackEffect & sse) override;//called when a specific effect is set to stacks
	//void battleTriggerEffect(const BattleTriggerEffect & bte) override;
	void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) override; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom) override; //called when stacks are healed / resurrected first element of pair - stack id, second - healed hp
	void battleNewStackAppeared(const CStack * stack) override; //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	void battleObstaclesRemoved(const std::set<si32> & removedObstacles) override; //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	void battleCatapultAttacked(const CatapultAttack & ca) override; //called when catapult makes an attack
	void battleStacksRemoved(const BattleStacksRemoved & bsr) override; //called when certain stack is completely removed from battlefield

	BattleAction goTowards(const CStack * stack, BattleHex hex );

	virtual void saveGame(BinarySerializer & h, const int version) override;
	virtual void loadGame(BinaryDeserializer & h, const int version) override;

};

