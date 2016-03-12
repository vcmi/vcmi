#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "BattleAI.h"
#include "../../lib/BattleState.h"
#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/VCMI_Lib.h"

using boost::optional;
static std::shared_ptr<CBattleCallback> cbc;

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

struct Priorities
{
	double manaValue;
	double generalResourceValueModifier;
	std::vector<double> resourceTypeBaseValues;
	std::function<double(const CStack *)> stackEvaluator;


	Priorities()
	{
		manaValue = 0.;
		generalResourceValueModifier = 1.;
		range::copy(VLC->objh->resVals, std::back_inserter(resourceTypeBaseValues));
		stackEvaluator = [](const CStack*){ return 1.0; };
	}
};

Priorities *priorities = nullptr;


namespace {

int distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances& dists, BattleHex *chosenHex = nullptr)
{
	int ret = 1000000;
	for(BattleHex n : hex.neighbouringTiles())
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

}

template <typename Container, typename Pred>
auto sum(const Container & c, Pred p) -> decltype(p(*std::begin(c)))
{
	double ret = 0;
	for(const auto &element : c)
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

void CBattleAI::init(std::shared_ptr<CBattleCallback> CB)
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
	return !cbc->battleIsFinished();
}

BattleAction CBattleAI::activeStack( const CStack * stack )
{
	LOG_TRACE_PARAMS(logAi, "stack: %s", stack->nodeName())	;

	cbc = cb; //TODO: make solid sure that AIs always use their callbacks (need to take care of event handlers too)
	try
	{
		print("activeStack called for " + stack->nodeName());
		if(stack->type->idNumber == CreatureID::CATAPULT)
			return useCatapult(stack);

		if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON) && stack->hasBonusOfType(Bonus::HEALER))
		{
			auto healingTargets = cb->battleGetStacks(CBattleInfoEssentials::ONLY_MINE);
			std::map<int, const CStack*> woundHpToStack;
			for(auto stack : healingTargets)
				if(auto woundHp = stack->MaxHealth() - stack->firstHPleft)
					woundHpToStack[woundHp] = stack;

			if(woundHpToStack.empty())
				return BattleAction::makeDefend(stack);
			else
				return BattleAction::makeHeal(stack, woundHpToStack.rbegin()->second); //last element of the woundHpToStack is the most wounded stack
		}

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
				ThreatMap threatsToUs(stack);
				auto dists = cbc->battleGetDistances(stack);
				const EnemyInfo &ei= *range::min_element(targets.unreachableEnemies, std::bind(isCloser, _1, _2, std::ref(dists)));
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
	vstd::copy_if(VLC->spellh->objects, std::back_inserter(possibleSpells), [this] (const CSpell *s) -> bool
	{
		auto problem = cbc->battleCanCastThisSpell(s);
		return problem == ESpellCastProblem::OK;
	});
	LOGFL("I can cast %d spells.", possibleSpells.size());

	vstd::erase_if(possibleSpells, [](const CSpell *s)
	{return spellType(s) == OTHER; });
	LOGFL("I know about workings of %d of them.", possibleSpells.size());

	//Get possible spell-target pairs
	std::vector<PossibleSpellcast> possibleCasts;
	for(auto spell : possibleSpells)
	{
		for(auto hex : getTargetsToConsider(spell))
		{
			PossibleSpellcast ps = {spell, hex};
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return;

	std::map<const CStack*, int> valueOfStack;
	for(auto stack : cb->battleGetStacks())
	{
		PotentialTargets pt(stack);
		valueOfStack[stack] = pt.bestActionValue();
	}

	auto evaluateSpellcast = [&] (const PossibleSpellcast &ps) -> int
	{
		const int skillLevel = hero->getSpellSchoolLevel(ps.spell);
		const int spellPower = hero->getPrimSkillLevel(PrimarySkill::SPELL_POWER);

		switch(spellType(ps.spell))
		{
		case OFFENSIVE_SPELL:
			{
				int damageDealt = 0, damageReceived = 0;
				
				auto stacksSuffering = ps.spell->getAffectedStacks(cb.get(), ECastingMode::HERO_CASTING, playerID, skillLevel, ps.dest, hero);

				if(stacksSuffering.empty())
					return -1;

				for(auto stack : stacksSuffering)
				{
					const int dmg = ps.spell->calculateDamage(hero, stack, skillLevel, spellPower);
					if(stack->owner == playerID)
						damageReceived += dmg;
					else
						damageDealt += dmg;
				}

				const int damageDiff = damageDealt - damageReceived;


				LOGFL("Casting %s on hex %d would deal %d damage points among %d stacks.",
					ps.spell->name % ps.dest % damageDiff % stacksSuffering.size());
				//TODO tactic effect too
				return damageDiff;
			}
		case TIMED_EFFECT:
			{
				StackWithBonuses swb;
				swb.stack = cb->battleGetStackByPos(ps.dest);
				if(!swb.stack)
					return -1;

				Bonus pseudoBonus;
				pseudoBonus.sid = ps.spell->id;
				pseudoBonus.val = skillLevel;
				pseudoBonus.turnsRemain = 1; //TODO
				CStack::stackEffectToFeature(swb.bonusesToAdd, pseudoBonus);

				HypotheticChangesToBattleState state;
				state.bonusesOfStacks[swb.stack] = &swb;

				PotentialTargets pt(swb.stack, state);
				auto newValue = pt.bestActionValue();
				auto oldValue = valueOfStack[swb.stack];
				auto gain = newValue - oldValue;
				if(swb.stack->owner != playerID) //enemy
					gain = -gain;

				LOGFL("Casting %s on %s would improve the stack by %d points (from %d to %d)",
					ps.spell->name % swb.stack->nodeName() % gain % (oldValue) % (newValue));

				return gain;
			}
		default:
			assert(0);
			return 0;
		}
	};

	auto castToPerform = *vstd::maxElementByFun(possibleCasts, evaluateSpellcast);
	LOGFL("Best spell is %s. Will cast.", castToPerform.spell->name);

	BattleAction spellcast;
	spellcast.actionType = Battle::HERO_SPELL;
	spellcast.additionalInfo = castToPerform.spell->id;
	spellcast.destinationTile = castToPerform.dest;
	spellcast.side = side;
	spellcast.stackNumber = (!side) ? -1 : -2;

	cb->battleMakeAction(&spellcast);
}

std::vector<BattleHex> CBattleAI::getTargetsToConsider( const CSpell *spell ) const
{
	if(spell->getTargetType() == CSpell::NO_TARGET)
	{
		//Spell can be cast anywhere, all hexes are potentially considerable.
		std::vector<BattleHex> ret;

		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			if(BattleHex(i).isAvailable())
				ret.push_back(i);

		return ret;
	}
	else
	{
		//TODO when massive effect -> doesn't matter where cast
		return cbc->battleGetPossibleTargets(playerID, spell);
	}
}

boost::optional<BattleAction> CBattleAI::considerFleeingOrSurrendering()
{
	if(cb->battleCanSurrender(playerID))
	{

	}
	if(cb->battleCanFlee())
	{

	}
	return boost::none;
}

ThreatMap::ThreatMap(const CStack *Endangered) : endangered(Endangered)
{
	sufferedDamage.fill(0);

	for(const CStack *enemy : cbc->battleGetStacks())
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
				for(auto n : BattleHex(i).neighbouringTiles())
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

const TBonusListPtr StackWithBonuses::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root /*= nullptr*/, const std::string &cachingStr /*= ""*/) const
{
	TBonusListPtr ret = std::make_shared<BonusList>();
	const TBonusListPtr originalList = stack->getAllBonuses(selector, limit, root, cachingStr);
	range::copy(*originalList, std::back_inserter(*ret));
	for(auto &bonus : bonusesToAdd)
	{
		if(selector(&bonus)  &&  (!limit || !limit(&bonus)))
			ret->push_back(&bonus);
	}

	//TODO limiters?

	return ret;
}

int AttackPossibility::damageDiff() const
{
	if (!priorities)
		priorities = new Priorities;
	const auto dealtDmgValue = priorities->stackEvaluator(enemy) * damageDealt;
	const auto receivedDmgValue = priorities->stackEvaluator(attack.attacker) * damageReceived;
	return dealtDmgValue - receivedDmgValue;
}

int AttackPossibility::attackValue() const
{
	return damageDiff() + tacticImpact;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo &AttackInfo, const HypotheticChangesToBattleState &state, BattleHex hex)
{
	auto attacker = AttackInfo.attacker;
	auto enemy = AttackInfo.defender;

	const int remainingCounterAttacks = getValOr(state.counterAttacksLeft, enemy, enemy->counterAttacksRemaining());
	const bool counterAttacksBlocked = attacker->hasBonusOfType(Bonus::BLOCKS_RETALIATION) || enemy->hasBonusOfType(Bonus::NO_RETALIATION);
	const int totalAttacks = 1 + AttackInfo.attackerBonuses->getBonuses(Selector::type(Bonus::ADDITIONAL_ATTACK), (Selector::effectRange (Bonus::NO_LIMIT).Or(Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))))->totalValue();

	AttackPossibility ap = {enemy, hex, AttackInfo, 0, 0, 0};

	auto curBai = AttackInfo; //we'll modify here the stack counts
	for(int i  = 0; i < totalAttacks; i++)
	{
		std::pair<ui32, ui32> retaliation(0,0);
		auto attackDmg = cbc->battleEstimateDamage(curBai, &retaliation);
		ap.damageDealt = (attackDmg.first + attackDmg.second) / 2;
		ap.damageReceived = (retaliation.first + retaliation.second) / 2;

		if(remainingCounterAttacks <= i || counterAttacksBlocked)
			ap.damageReceived = 0;

		curBai.attackerCount = attacker->count - attacker->countKilledByAttack(ap.damageReceived).first;
		curBai.defenderCount = enemy->count - enemy->countKilledByAttack(ap.damageDealt).first;
		if(!curBai.attackerCount)
			break;
		//TODO what about defender? should we break? but in pessimistic scenario defender might be alive
	}

	//TODO other damage related to attack (eg. fire shield and other abilities)

	//Limit damages by total stack health
	vstd::amin(ap.damageDealt, enemy->count * enemy->MaxHealth() - (enemy->MaxHealth() - enemy->firstHPleft));
	vstd::amin(ap.damageReceived, attacker->count * attacker->MaxHealth() - (attacker->MaxHealth() - attacker->firstHPleft));

	return ap;
}


PotentialTargets::PotentialTargets(const CStack *attacker, const HypotheticChangesToBattleState &state /*= HypotheticChangesToBattleState()*/)
{
	auto dists = cbc->battleGetDistances(attacker);
	auto avHexes = cbc->battleGetAvailableHexes(attacker, false);

	for(const CStack *enemy : cbc->battleGetStacks())
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
			for(BattleHex hex : avHexes)
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

	return *vstd::maxElementByFun(possibleAttacks, [](const AttackPossibility &ap) { return ap.attackValue(); } );
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
