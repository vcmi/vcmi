#pragma once

#include "../../lib/BattleHex.h"
#include "../../lib/HeroBonus.h"
#include "../../lib/CBattleCallback.h"

class CSpell;


class StackWithBonuses : public IBonusBearer
{
public:
	const CStack *stack;
	mutable std::vector<Bonus> bonusesToAdd;

	virtual const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = NULL, const std::string &cachingStr = "") const OVERRIDE;
};

struct EnemyInfo
{
	const CStack * s;
	int adi, adr;
	std::vector<BattleHex> attackFrom; //for melee fight
	EnemyInfo(const CStack * _s) : s(_s)
	{}
	void calcDmg(const CStack * ourStack);

	bool operator==(const EnemyInfo& ei) const
	{
		return s == ei.s;
	}
};


//FIXME: unused function
/*
static bool willSecondHexBlockMoreEnemyShooters(const BattleHex &h1, const BattleHex &h2)
{
	int shooters[2] = {0}; //count of shooters on hexes

	for(int i = 0; i < 2; i++)
		BOOST_FOREACH(BattleHex neighbour, (i ? h2 : h1).neighbouringTiles())
			if(const CStack *s = cbc->battleGetStackByPos(neighbour))
				if(s->getCreature()->isShooting())
						shooters[i]++;

	return shooters[0] < shooters[1];
}
*/



struct ThreatMap
{	
	std::array<std::vector<BattleAttackInfo>, GameConstants::BFIELD_SIZE> threatMap; // [hexNr] -> enemies able to strike
	
	const CStack *endangered; 
	std::array<int, GameConstants::BFIELD_SIZE> sufferedDamage; 

	ThreatMap(const CStack *Endangered);
};

struct HypotheticChangesToBattleState
{
	std::map<const CStack *, const IBonusBearer *> bonusesOfStacks;
	std::map<const CStack *, int> counterAttacksLeft;
};

struct AttackPossibility
{
	const CStack *enemy; //redundant (to attack.defender) but looks nice
	BattleHex tile; //tile from which we attack
	BattleAttackInfo attack;

	int damageDealt;
	int damageReceived; //usually by counter-attack
	int tacticImpact;

	int damageDiff() const;
	int attackValue() const;

	static AttackPossibility evaluate(const BattleAttackInfo &AttackInfo, const HypotheticChangesToBattleState &state, BattleHex hex);
};

template<typename Key, typename Val, typename Val2>
const Val &getValOr(const std::map<Key, Val> &Map, const Key &key, const Val2 &defaultValue)
{
	auto i = Map.find(key);
	if(i != Map.end())
		return i->second;
	else 
		return defaultValue;
}

struct PotentialTargets
{
	std::vector<AttackPossibility> possibleAttacks;
	std::vector<const CStack *> unreachableEnemies;

	//std::function<AttackPossibility(bool,BattleHex)>  GenerateAttackInfo; //args: shooting, destHex

	PotentialTargets(){};
	PotentialTargets(const CStack *attacker, const HypotheticChangesToBattleState &state = HypotheticChangesToBattleState());

	AttackPossibility bestAction() const;
	int bestActionValue() const;
};

class CBattleAI : public CBattleGameInterface
{
	int side;
	shared_ptr<CBattleCallback> cb;
	
	//Previous setting of cb 
	bool wasWaitingForRealize, wasUnlockingGs;

	void print(const std::string &text) const;
public:
	CBattleAI(void);
	~CBattleAI(void);

	void init(shared_ptr<CBattleCallback> CB) OVERRIDE;
	void actionFinished(const BattleAction &action) OVERRIDE;//occurs AFTER every action taken by any stack or by the hero
	void actionStarted(const BattleAction &action) OVERRIDE;//occurs BEFORE every action taken by any stack or by the hero
	BattleAction activeStack(const CStack * stack) OVERRIDE; //called when it's turn of that stack

	void battleAttack(const BattleAttack *ba) OVERRIDE; //called when stack is performing attack
	void battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa) OVERRIDE; //called when stack receives damage (after battleAttack())
	void battleEnd(const BattleResult *br) OVERRIDE;
	//void battleResultsApplied() OVERRIDE; //called when all effects of last battle are applied
	void battleNewRoundFirst(int round) OVERRIDE; //called at the beginning of each turn before changes are applied;
	void battleNewRound(int round) OVERRIDE; //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	void battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance) OVERRIDE;
	void battleSpellCast(const BattleSpellCast *sc) OVERRIDE;
	void battleStacksEffectsSet(const SetStackEffect & sse) OVERRIDE;//called when a specific effect is set to stacks
	//void battleTriggerEffect(const BattleTriggerEffect & bte) OVERRIDE;
	void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) OVERRIDE; //called by engine when battle starts; side=0 - left, side=1 - right
	void battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom) OVERRIDE; //called when stacks are healed / resurrected first element of pair - stack id, second - healed hp
	void battleNewStackAppeared(const CStack * stack) OVERRIDE; //not called at the beginning of a battle or by resurrection; called eg. when elemental is summoned
	void battleObstaclesRemoved(const std::set<si32> & removedObstacles) OVERRIDE; //called when a certain set  of obstacles is removed from batlefield; IDs of them are given
	void battleCatapultAttacked(const CatapultAttack & ca) OVERRIDE; //called when catapult makes an attack
	void battleStacksRemoved(const BattleStacksRemoved & bsr) OVERRIDE; //called when certain stack is completely removed from battlefield

	BattleAction goTowards(const CStack * stack, BattleHex hex );
	BattleAction useCatapult(const CStack * stack);

	boost::optional<BattleAction> considerFleeingOrSurrendering();

	void attemptCastingSpell();
	std::vector<BattleHex> getTargetsToConsider(const CSpell *spell) const;
};

