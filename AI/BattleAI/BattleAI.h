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

	virtual const TBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;
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
const Val getValOr(const std::map<Key, Val> &Map, const Key &key, const Val2 defaultValue)
{
	//returning references here won't work: defaultValue must be converted into Val, creating temporary
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
	std::shared_ptr<CBattleCallback> cb;

	//Previous setting of cb
	bool wasWaitingForRealize, wasUnlockingGs;

	void print(const std::string &text) const;
public:
	CBattleAI(void);
	~CBattleAI(void);

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
	BattleAction useCatapult(const CStack * stack);

	boost::optional<BattleAction> considerFleeingOrSurrendering();

	void attemptCastingSpell();
	std::vector<BattleHex> getTargetsToConsider(const CSpell *spell, const ISpellCaster * caster) const;
};

