#pragma once

class CStupidAI : public CBattleGameInterface
{
	int side;
	CBattleCallback *cb;

	void print(const std::string &text) const;
public:
	CStupidAI(void);
	~CStupidAI(void);

	void init(CBattleCallback * CB) OVERRIDE;
	void actionFinished(const BattleAction *action) OVERRIDE;//occurs AFTER every action taken by any stack or by the hero
	void actionStarted(const BattleAction *action) OVERRIDE;//occurs BEFORE every action taken by any stack or by the hero
	BattleAction activeStack(const CStack * stack) OVERRIDE; //called when it's turn of that stack

	void battleAttack(const BattleAttack *ba) OVERRIDE; //called when stack is performing attack
	void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) OVERRIDE; //called when stack receives damage (after battleAttack())
	void battleEnd(const BattleResult *br) OVERRIDE;
	//void battleResultsApplied() OVERRIDE; //called when all effects of last battle are applied
	void battleNewRoundFirst(int round) OVERRIDE; //called at the beginning of each turn before changes are applied;
	void battleNewRound(int round) OVERRIDE; //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(const CStack * stack, THex dest, int distance, bool end) OVERRIDE;
	void battleSpellCast(const BattleSpellCast *sc) OVERRIDE;
	void battleStacksEffectsSet(const SetStackEffect & sse) OVERRIDE;//called when a specific effect is set to stacks
	void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) OVERRIDE; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, si32 lifeDrainFrom) OVERRIDE; //called when stacks are healed / resurrected first element of pair - stack id, second - healed hp
	void battleNewStackAppeared(const CStack * stack) OVERRIDE; //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	void battleObstaclesRemoved(const std::set<si32> & removedObstacles) OVERRIDE; //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	void battleCatapultAttacked(const CatapultAttack & ca) OVERRIDE; //called when catapult makes an attack
	void battleStacksRemoved(const BattleStacksRemoved & bsr) OVERRIDE; //called when certain stack is completely removed from battlefield

	BattleAction goTowards(const CStack * stack, THex hex );
};

