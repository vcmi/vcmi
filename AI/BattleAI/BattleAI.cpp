#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "BattleAI.h"
#include "../../lib/BattleState.h"
#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../../lib/VCMI_Lib.h"

using boost::optional;
shared_ptr<CBattleCallback> cbc;
const CBattleAI *ai;

template<class ForwardRange, class ValueFunction>
auto maxElementByFun(const ForwardRange& rng, ValueFunction vf) -> decltype(boost::begin(rng))
{
	std::map<double, decltype(boost::begin(rng))> elements;
	for(auto i = boost::begin(rng); i != boost::end(rng); i++)
		elements[vf(*i)] = i;

	return (--(elements.end()))->second;
}


#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))




int distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances& dists, BattleHex *chosenHex = NULL)
{
	int ret = 1000000;
	BOOST_FOREACH(BattleHex n, hex.neighbouringTiles())
	{
		if(dists[n] >= 0 && dists[n] < ret)
		{
			ret = dists[n];
			if(chosenHex)
				*chosenHex = n;
		}
	}

	return ret;
}

bool isCloser(const EnemyInfo & ei1, const EnemyInfo & ei2, const ReachabilityInfo::TDistances & dists)
{
	return distToNearestNeighbour(ei1.s->position, dists) < distToNearestNeighbour(ei2.s->position, dists);
}

template <typename Container, typename Pred>
auto sum(const Container & c, Pred p) -> decltype(p(*boost::begin(c)))
{
	double ret = 0;
	BOOST_FOREACH(const auto &element, c)
	{
		ret += p(element);
	}

	return ret;
}



CBattleAI::CBattleAI(void)
	: side(-1)
{
	print("created");
}


CBattleAI::~CBattleAI(void)
{
	print("destroyed");

	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
		cb->unlockGsWhenWaiting = wasUnlockingGs;
	}
}

void CBattleAI::init(shared_ptr<CBattleCallback> CB)
{
	print("init called, saving ptr to IBattleCallback");
	cbc = cb = CB;
	playerID = *CB->getPlayerID();; //TODO should be sth in callback

	wasWaitingForRealize = cb->waitTillRealize;
	wasUnlockingGs = CB->unlockGsWhenWaiting;
	CB->waitTillRealize = true;
	CB->unlockGsWhenWaiting = false;
}

static bool thereRemainsEnemy()
{
	return cbc->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY).size();
}

BattleAction CBattleAI::activeStack( const CStack * stack )
{
	LOG_TRACE_PARAMS(logAi, "stack: %s", stack->nodeName())	;

	cbc = cb; //TODO: make solid sure that AIs always use their callbacks (need to take care of event handlers too)
	ai = this;

	try
	{
		print("activeStack called for " + stack->nodeName());
		if(stack->type->idNumber == CreatureID::CATAPULT)
			return useCatapult(stack);

		print("Evaluating tactic situation...");
		tacticInfo = make_unique<TacticInfo>();
		print("Done!");

		if(cb->battleCanCastSpell())
			attemptCastingSpell();

		if(!thereRemainsEnemy())
			return BattleAction();

		if(auto action = considerFleeingOrSurrendering())
			return *action;

		if(cb->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY).empty())
		{
			//We apparently won battle by casting spell, return defend... (accessing cb may cause trouble)
			return BattleAction::makeDefend(stack);
		}

		PotentialTargets targets(stack);


		if(targets.possibleAttacks.size())
		{
			auto hlp = targets.bestAction();
			if(hlp.attack.shooting)
				return BattleAction::makeShotAttack(stack, hlp.enemy);
			else
				return BattleAction::makeMeleeAttack(stack, hlp.enemy, hlp.tile);
		}
		else
		{
			if(stack->waited())
			{
				auto dists = cbc->battleGetDistances(stack);
				const EnemyInfo &ei= *range::min_element(targets.unreachableEnemies, boost::bind(isCloser, _1, _2, boost::ref(dists)));
				if(distToNearestNeighbour(ei.s->position, dists) < GameConstants::BFIELD_SIZE)
				{
					return goTowards(stack, ei.s->position);
				}
			}
			else
			{
				return BattleAction::makeWait(stack);
			}
		}
	}
	catch(std::exception &e)
	{
		logAi->errorStream() << "Exception occurred in " << __FUNCTION__ << " " << e.what();
	}

	return BattleAction::makeDefend(stack);
}

void CBattleAI::actionFinished(const BattleAction &action)
{
	print("actionFinished called");
}

void CBattleAI::actionStarted(const BattleAction &action)
{
	print("actionStarted called");
}

void CBattleAI::battleAttack(const BattleAttack *ba)
{
	print("battleAttack called");
}

void CBattleAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	print("battleStacksAttacked called");
}

void CBattleAI::battleEnd(const BattleResult *br)
{
	print("battleEnd called");
}

void CBattleAI::battleNewRoundFirst(int round)
{
	print("battleNewRoundFirst called");
}

void CBattleAI::battleNewRound(int round)
{
	print("battleNewRound called");
}

void CBattleAI::battleStackMoved(const CStack * stack, std::vector<BattleHex> dest, int distance)
{
	print("battleStackMoved called");;
}

void CBattleAI::battleSpellCast(const BattleSpellCast *sc)
{
	print("battleSpellCast called");
}

void CBattleAI::battleStacksEffectsSet(const SetStackEffect & sse)
{
	print("battleStacksEffectsSet called");
}

void CBattleAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side)
{
	print("battleStart called");
	side = Side;
}

void CBattleAI::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom)
{
	print("battleStacksHealedRes called");
}

void CBattleAI::battleNewStackAppeared(const CStack * stack)
{
	print("battleNewStackAppeared called");
}

void CBattleAI::battleObstaclesRemoved(const std::set<si32> & removedObstacles)
{
	print("battleObstaclesRemoved called");
}

void CBattleAI::battleCatapultAttacked(const CatapultAttack & ca)
{
	print("battleCatapultAttacked called");
}

void CBattleAI::battleStacksRemoved(const BattleStacksRemoved & bsr)
{
	print("battleStacksRemoved called");
}

void CBattleAI::print(const std::string &text) const
{
    logAi->traceStream() << "CBattleAI [" << this <<"]: " << text;
}

BattleAction CBattleAI::goTowards(const CStack * stack, BattleHex destination)
{
	assert(destination.isValid());
	auto avHexes = cb->battleGetAvailableHexes(stack, false);
	auto reachability = cb->getReachability(stack);

	if(vstd::contains(avHexes, destination))
		return BattleAction::makeMove(stack, destination);

	auto destNeighbours = destination.neighbouringTiles();
	if(vstd::contains_if(destNeighbours, [&](BattleHex n) { return stack->coversPos(destination); }))
	{
        logAi->warnStream() << "Warning: already standing on neighbouring tile!";
		//We shouldn't even be here...
		return BattleAction::makeDefend(stack);
	}

	vstd::erase_if(destNeighbours, [&](BattleHex hex){ return !reachability.accessibility.accessible(hex, stack); });

	if(!avHexes.size() || !destNeighbours.size()) //we are blocked or dest is blocked
	{
		print("goTowards: Stack cannot move! That's " + stack->nodeName());
		return BattleAction::makeDefend(stack);
	}

	if(stack->hasBonusOfType(Bonus::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto distToDestNeighbour = [&](BattleHex hex) -> int
		{
			auto nearestNeighbourToHex = vstd::minElementByFun(destNeighbours, [&](BattleHex a)
			{
				return BattleHex::getDistance(a, hex);
			});

			return BattleHex::getDistance(*nearestNeighbourToHex, hex);
		};

		auto nearestAvailableHex = vstd::minElementByFun(avHexes, distToDestNeighbour);
		return BattleAction::makeMove(stack, *nearestAvailableHex);
	}
	else
	{
		BattleHex bestNeighbor = destination;
		if(distToNearestNeighbour(destination, reachability.distances, &bestNeighbor) > GameConstants::BFIELD_SIZE)
		{
			print("goTowards: Cannot reach");
			return BattleAction::makeDefend(stack);
		}

		BattleHex currentDest = bestNeighbor;
		while(1)
		{
			assert(currentDest.isValid());
			if(vstd::contains(avHexes, currentDest))
				return BattleAction::makeMove(stack, currentDest);

			currentDest = reachability.predecessors[currentDest];
		}
	}
}

BattleAction CBattleAI::useCatapult(const CStack * stack)
{
	throw std::runtime_error("The method or operation is not implemented.");
}

enum SpellTypes
{
	OFFENSIVE_SPELL, TIMED_EFFECT, OTHER
};

SpellTypes spellType(const CSpell *spell)
{
	if (spell->isOffensiveSpell())
		return OFFENSIVE_SPELL;
	if (spell->hasEffects())
		return TIMED_EFFECT;	
	return OTHER;

}

struct PossibleSpellcast
{
	const CSpell *spell;
	BattleHex dest;
};

struct CurrentOffensivePotential
{
	std::map<const CStack *, PotentialTargets> ourAttacks;
	std::map<const CStack *, PotentialTargets> enemyAttacks;

	CurrentOffensivePotential(ui8 side)
	{
		BOOST_FOREACH(auto stack, cbc->battleGetStacks())
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
		BOOST_FOREACH(auto &p, ourAttacks)
			ourPotential += p.second.bestAction().attackValue();

		BOOST_FOREACH(auto &p, enemyAttacks)
			enemyPotential += p.second.bestAction().attackValue();

		return ourPotential - enemyPotential;
	}
};


// 
// //set has its own order, so remove_if won't work. TODO - reuse for map
// template<typename Elem, typename Predicate>
// void erase_if(std::set<Elem> &setContainer, Predicate pred)
// {
// 	auto itr = setContainer.begin();
// 	auto endItr = setContainer.end(); 
// 	while(itr != endItr)
// 	{
// 		auto tmpItr = itr++;
// 		if(pred(*tmpItr))
// 			setContainer.erase(tmpItr);
// 	}
// }



void CBattleAI::attemptCastingSpell()
{
	LOGL("Casting spells sounds like fun. Let's see...");

	auto hero = cb->battleGetMyHero();

	//auto known = cb->battleGetFightingHero(side);

	//Get all spells we can cast
	std::vector<const CSpell*> possibleSpells;
	vstd::copy_if(VLC->spellh->spells, std::back_inserter(possibleSpells), [this] (const CSpell *s) -> bool
	{ 
		auto problem = cbc->battleCanCastThisSpell(s); 
		return problem == ESpellCastProblem::OK; 
	});
	LOGFL("I can cast %d spells.", possibleSpells.size());

	vstd::erase_if(possibleSpells, [](const CSpell *s) 
	{ return spellType(s) == OTHER; });
	LOGFL("I know about workings of %d of them.", possibleSpells.size());

	//Get possible spell-target pairs
	std::vector<PossibleSpellcast> possibleCasts;
	BOOST_FOREACH(auto spell, possibleSpells)
	{
		BOOST_FOREACH(auto hex, getTargetsToConsider(spell))
		{
			PossibleSpellcast ps = {spell, hex};
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return;
	
	auto evaluateSpellcast = [&] (const PossibleSpellcast &ps) -> int
	{
		const int skillLevel = hero->getSpellSchoolLevel(ps.spell);
		const int spellPower = hero->getPrimSkillLevel(PrimarySkill::SPELL_POWER);

		switch(spellType(ps.spell))
		{
		case OFFENSIVE_SPELL:
			{
				int damageDealt = 0, damageReceived = 0;

				auto stacksSuffering = cb->getAffectedCreatures(ps.spell, skillLevel, playerID, ps.dest);
				vstd::erase_if(stacksSuffering, [&](const CStack *stack) -> bool
				{
					return cb->battleIsImmune(hero, ps.spell, ECastingMode::HERO_CASTING, ps.dest);
				});

				if(stacksSuffering.empty())
					return -1;

				BOOST_FOREACH(auto stack, stacksSuffering)
				{
					const int dmg = cb->calculateSpellDmg(ps.spell, hero, stack, skillLevel, spellPower);
					if(stack->owner == playerID)
						damageReceived += priorities.stackEvaluator(stack) * dmg;
					else
						damageDealt += priorities.stackEvaluator(stack) * dmg;
				}

				const int damageDiff = damageDealt - damageReceived;


				LOGFL("Casting %s on hex %d would deal %d damage points among %d stacks.",
					ps.spell->name % ps.dest % damageDiff % stacksSuffering.size());

				return damageDiff;
			}
		case TIMED_EFFECT:
			{

				auto affectedCreatures = cb->getAffectedCreatures(ps.spell, skillLevel, playerID, ps.dest);
				if(affectedCreatures.empty())
					return -1;

				int baseValue = 0;
				BOOST_FOREACH(auto &affectedStack, affectedCreatures)
				{
					StackWithBonuses swb;
					swb.stack = affectedStack;

					Bonus pseudoBonus;
					pseudoBonus.sid = ps.spell->id;
					pseudoBonus.val = skillLevel;
					pseudoBonus.turnsRemain = 1; //TODO
					CStack::stackEffectToFeature(swb.bonusesToAdd, pseudoBonus);

					HypotheticChangesToBattleState state;
					state.bonusesOfStacks[swb.stack] = &swb;

					PotentialTargets pt(swb.stack, state);
					auto newValue = pt.bestActionValue();
					auto oldValue = tacticInfo->targets[swb.stack].bestActionValue();
					auto gain = newValue - oldValue;
					if(swb.stack->owner != playerID) //enemy
						gain = -gain;

					baseValue += gain;
					LOGFL("Casting %s on %s would improve the stack by %d points (from %d to %d)",
						ps.spell->name % swb.stack->nodeName() % gain % (oldValue) % (newValue));
				}


				const int time = std::min(spellPower, tacticInfo->expectedLength());
				int ret = 0;
				for(int i = 0; i < time; i++)
					ret += (baseValue>>i);

				LOGFL("Spell %s has base value of %d, will last %d turns of which %d is useful. Total value: %d.",
					ps.spell->name % baseValue % spellPower % time % ret);
				return ret;
			}
		default:
			assert(0);
			return 0;
		}
	};

	const auto castToPerform = *maxElementByFun(possibleCasts, evaluateSpellcast);
	const double spellValue = evaluateSpellcast(castToPerform);
	const double spellCost = priorities.manaValue * cb->battleGetSpellCost(castToPerform.spell, hero);
	LOGFL("Best spell is %s. Value is %d, cost %s.", castToPerform.spell->name % spellValue % spellCost);

	if(spellValue >= spellCost)
	{
		LOGL("Will cast the spell.");
		BattleAction spellcast;
		spellcast.actionType = Battle::HERO_SPELL;
		spellcast.additionalInfo = castToPerform.spell->id;
		spellcast.destinationTile = castToPerform.dest;
		spellcast.side = side;
		spellcast.stackNumber = (!side) ? -1 : -2;

		cb->battleMakeAction(&spellcast);
	}
	else
	{
		LOGL("Won't cast the spell.");
		return;
	}
}

std::vector<BattleHex> CBattleAI::getTargetsToConsider( const CSpell *spell ) const
{
	if(spell->getTargetType() == CSpell::NO_TARGET)
	{
		//Spell can be casted anywhere, all hexes are potentially considerable.
		std::vector<BattleHex> ret;

		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			if(BattleHex(i).isAvailable())
				ret.push_back(i);

		return ret;
	}
	else
	{
		//TODO when massive effect -> doesnt matter where cast
		return cbc->battleGetPossibleTargets(playerID, spell);
	}
}

boost::optional<BattleAction> CBattleAI::considerFleeingOrSurrendering()
{
	if(tacticInfo->expectedLength() >= 2  || tacticInfo->totalValue() > 0)
		return boost::none;

	BattleAction ba;
	ba.side = cb->battleGetMySide();
	ba.stackNumber = cb->battleActiveStack()->ID;

	if(cb->battleCanSurrender(playerID))
	{
		double armyValue = 0;
		BOOST_FOREACH(auto stack, cb->battleGetStacks(CBattleInfoEssentials::ONLY_MINE))
		{
			armyValue += stack->count*stack->MaxHealth()*priorities.stackEvaluator(stack);
		}

		double cost = cb->battleGetSurrenderCost() * priorities.generalResourceValueModifier * priorities.resourceTypeBaseValues[Res::GOLD];
		if(armyValue + priorities.heroValue > cost + tacticInfo->ourPotential)
		{
			ba.actionType = Battle::SURRENDER;
			return ba;
		}
	}
	if(cb->battleCanFlee())
	{
		if(priorities.heroValue > tacticInfo->ourPotential)
		{
			ba.actionType = Battle::RETREAT;
			return ba;
		}
	}
	return boost::none;
}

void CBattleAI::setPriorities(const Priorities &priorities)
{
	this->priorities = priorities;
}

ThreatMap::ThreatMap(const CStack *Endangered) : endangered(Endangered)
{
	sufferedDamage.fill(0);

	BOOST_FOREACH(const CStack *enemy, cbc->battleGetStacks())
	{
		//Consider only stacks of different owner
		if(enemy->attackerOwned == endangered->attackerOwned)
			continue;

		//Look-up which tiles can be melee-attacked
		std::array<bool, GameConstants::BFIELD_SIZE> meleeAttackable;
		meleeAttackable.fill(false);
		auto enemyReachability = cbc->getReachability(enemy);
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			if(enemyReachability.isReachable(i))
			{
				meleeAttackable[i] = true;
				BOOST_FOREACH(auto n, BattleHex(i).neighbouringTiles())
					meleeAttackable[n] = true;
			}
		}

		//Gather possible assaults
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		{
			if(cbc->battleCanShoot(enemy, i))
				threatMap[i].push_back(BattleAttackInfo(enemy, endangered, true));
			else if(meleeAttackable[i])
			{
				BattleAttackInfo bai(enemy, endangered, false);
				bai.chargedFields = std::max(BattleHex::getDistance(enemy->position, i) - 1, 0); //TODO check real distance (BFS), not just metric
				threatMap[i].push_back(BattleAttackInfo(bai));
			}
		}
	}

	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		sufferedDamage[i] = sum(threatMap[i], [](const BattleAttackInfo &bai) -> int
		{
			auto dmg = cbc->calculateDmgRange(bai);
			return (dmg.first + dmg.second)/2;
		});
	}
}

const TBonusListPtr StackWithBonuses::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= NULL*/, const std::string &cachingStr /*= ""*/) const 
{
	TBonusListPtr ret = make_shared<BonusList>();
	const TBonusListPtr originalList = stack->getAllBonuses(selector, limit, root, cachingStr);
	boost::copy(*originalList, std::back_inserter(*ret));
	BOOST_FOREACH(auto &bonus, bonusesToAdd)
	{
		if(selector(&bonus)  &&  (!limit || !limit(&bonus)))
			ret->push_back(&bonus);
	}

	//TODO limiters?

	return ret;
}

int AttackPossibility::damageDiff() const
{
	const auto dealtDmgValue = ai->priorities.stackEvaluator(enemy) * damageDealt;
	const auto receivedDmgValue = ai->priorities.stackEvaluator(attack.attacker) * damageReceived;
	return dealtDmgValue - receivedDmgValue;
}

int AttackPossibility::attackValue() const
{
	return damageDiff() /*+ tacticImpact*/;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo &AttackInfo, const HypotheticChangesToBattleState &state, BattleHex hex)
{
	auto attacker = AttackInfo.attacker;
	auto defender = AttackInfo.defender;

	const int initialAttackerCount = getValOr(state.stackCount, attacker, attacker->count);
	const int initialDefenderCount = getValOr(state.stackCount, defender, defender->count);
	const int remainingCounterAttacks = getValOr(state.counterAttacksLeft, defender, defender->counterAttacks);
	const bool counterAttacksBlocked = attacker->hasBonusOfType(Bonus::BLOCKS_RETALIATION) || defender->hasBonusOfType(Bonus::NO_RETALIATION);
	const int totalAttacks = 1 + AttackInfo.attackerBonuses->getBonuses(Selector::type(Bonus::ADDITIONAL_ATTACK), (Selector::effectRange (Bonus::NO_LIMIT) || Selector::effectRange(Bonus::ONLY_MELEE_FIGHT)))->totalValue();

	AttackPossibility ap = {defender, hex, AttackInfo, 0, 0/*, 0*/};

	auto curBai = AttackInfo; //we'll modify here the stack counts
	for(int i  = 0; i < totalAttacks; i++)
	{
		std::pair<ui32, ui32> retaliation(0,0);
		auto attackDmg = cbc->battleEstimateDamage(curBai, &retaliation);
		ap.damageDealt = (attackDmg.first + attackDmg.second) / 2;
		ap.damageReceived = (retaliation.first + retaliation.second) / 2;

		if(remainingCounterAttacks <= i || counterAttacksBlocked)
			ap.damageReceived = 0;

		curBai.attackerCount = initialAttackerCount - attacker->countKilledByAttack(ap.damageReceived).first;
		curBai.defenderCount = initialDefenderCount - defender->countKilledByAttack(ap.damageDealt).first;
		if(!curBai.attackerCount) 
			break;
		//TODO what about defender? should we break? but in pessimistic scenario defender might be alive
	}

	double luck = curBai.attackerBonuses->LuckVal();
	if(luck > 0)
		ap.damageDealt *= (1 + luck/12.0);

	//TODO other damage related to attack (eg. fire shield and other abilities)

	//Limit damages by total stack health
	vstd::amin(ap.damageDealt, initialDefenderCount * defender->MaxHealth() - (defender->MaxHealth() - defender->firstHPleft));
	vstd::amin(ap.damageReceived, initialAttackerCount * attacker->MaxHealth() - (attacker->MaxHealth() - attacker->firstHPleft));

	return ap;
}


PotentialTargets::PotentialTargets(const CStack *attacker, const HypotheticChangesToBattleState &state /*= HypotheticChangesToBattleState()*/)
{
	auto dists = cbc->battleGetDistances(attacker);
	auto avHexes = cbc->battleGetAvailableHexes(attacker, false);

	BOOST_FOREACH(const CStack *enemy, cbc->battleGetStacks())
	{
		//Consider only stacks of different owner
		if(enemy->attackerOwned == attacker->attackerOwned)
			continue;

		auto GenerateAttackInfo = [&](bool shooting, BattleHex hex) -> AttackPossibility
		{
			auto bai = BattleAttackInfo(attacker, enemy, shooting);
			bai.attackerBonuses = getValOr(state.bonusesOfStacks, bai.attacker, bai.attacker);
			bai.defenderBonuses = getValOr(state.bonusesOfStacks, bai.defender, bai.defender);
			
			if(hex.isValid())
			{
				assert(dists[hex] <= attacker->Speed());
				bai.chargedFields = dists[hex];
			}

			return AttackPossibility::evaluate(bai, state, hex);
		};

		if(cbc->battleCanShoot(attacker, enemy->position))
		{
			possibleAttacks.push_back(GenerateAttackInfo(true, BattleHex::INVALID));
		}
		else
		{
			BOOST_FOREACH(BattleHex hex, avHexes)
				if(CStack::isMeleeAttackPossible(attacker, enemy, hex))
					possibleAttacks.push_back(GenerateAttackInfo(false, hex));

			if(!vstd::contains_if(possibleAttacks, [=](const AttackPossibility &pa) { return pa.enemy == enemy; }))
				unreachableEnemies.push_back(enemy);
		}
	}
}

AttackPossibility PotentialTargets::bestAction() const
{
	if(possibleAttacks.empty())
		throw std::runtime_error("No best action, since we don't have any actions");

	return *maxElementByFun(possibleAttacks, [](const AttackPossibility &ap) { return ap.attackValue(); } );
}

int PotentialTargets::bestActionValue() const
{
	if(possibleAttacks.empty())
		return 0;

	return bestAction().attackValue();
}

void EnemyInfo::calcDmg(const CStack * ourStack)
{
	TDmgRange retal, dmg = cbc->battleEstimateDamage(ourStack, s, &retal);
	adi = (dmg.first + dmg.second) / 2;
	adr = (retal.first + retal.second) / 2;
}

TacticInfo::TacticInfo(const HypotheticChangesToBattleState &state /*= HypotheticChangesToBattleState()*/)
{
	ourPotential = enemyPotential = ourHealth = enemyhealth = 0;

	BOOST_FOREACH(const CStack * ourStack, cbc->battleGetStacks(CBattleInfoEssentials::ONLY_MINE))
	{
		if(getValOr(state.stackCount, ourStack, ourStack->count) <= 0) continue;
		targets[ourStack] = PotentialTargets(ourStack, state);
		ourPotential += targets[ourStack].bestActionValue();
		ourHealth += (ourStack->count-1) * ourStack->MaxHealth() + ourStack->firstHPleft;
	}
	BOOST_FOREACH(const CStack * enemyStack, cbc->battleGetStacks(CBattleInfoEssentials::ONLY_ENEMY))
	{
		if(getValOr(state.stackCount, enemyStack, enemyStack->count) <= 0) continue;
		targets[enemyStack] = PotentialTargets(enemyStack, state);
		enemyPotential += targets[enemyStack].bestActionValue();
		enemyhealth += (enemyStack->count-1) * enemyStack->MaxHealth() + enemyStack->firstHPleft;
	}

	if(ourPotential < 0)
	{
		enemyPotential -= ourPotential;
		ourPotential = 0;
	}
	if(enemyPotential < 0)
	{
		ourPotential -= enemyPotential;
		enemyPotential = 0;
	}
}

double TacticInfo::totalValue() const
{
	return ourPotential - enemyPotential;
}

int TacticInfo::expectedLength() const
{
	double value = totalValue();
	if(value > 0)
		return std::ceil(enemyhealth / ourPotential);
	else if(value < 0)
		return std::ceil(ourHealth / enemyPotential);

	return 10000; 
}
