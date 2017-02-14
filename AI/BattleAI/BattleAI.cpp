/*
	 * BattleAI.cpp, part of VCMI engine
	 *
	 * Authors: listed in file AUTHORS in main folder
	 *
	 * License: GNU General Public License v2.0 or later
	 * Full text of license available in license.txt file, in main folder
	 *
	 */
#include "StdInc.h"
#include "BattleAI.h"
#include "StackWithBonuses.h"
#include "EnemyInfo.h"
#include "../../lib/spells/CSpellHandler.h"

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

CBattleAI::CBattleAI(void)
	: side(-1), wasWaitingForRealize(false), wasUnlockingGs(false)
{
}

CBattleAI::~CBattleAI(void)
{
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
		cb->unlockGsWhenWaiting = wasUnlockingGs;
	}
}

void CBattleAI::init(std::shared_ptr<CBattleCallback> CB)
{
	setCbc(CB);
	cb = CB;
	playerID = *CB->getPlayerID();; //TODO should be sth in callback
	wasWaitingForRealize = cb->waitTillRealize;
	wasUnlockingGs = CB->unlockGsWhenWaiting;
	CB->waitTillRealize = true;
	CB->unlockGsWhenWaiting = false;
}

BattleAction CBattleAI::activeStack( const CStack * stack )
{
	LOG_TRACE_PARAMS(logAi, "stack: %s", stack->nodeName())	;
	setCbc(cb); //TODO: make solid sure that AIs always use their callbacks (need to take care of event handlers too)
	try
	{
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

		attemptCastingSpell();

		if(auto ret = getCbc()->battleIsFinished())
		{
			//spellcast may finish battle
			//send special preudo-action
			BattleAction cancel;
			cancel.actionType = Battle::CANCEL;
			return cancel;
		}

		if(auto action = considerFleeingOrSurrendering())
			return *action;
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
				//ThreatMap threatsToUs(stack); // These lines may be usefull but they are't used in the code.
				auto dists = getCbc()->battleGetDistances(stack);
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
		logAi->error("Exception occurred in %s %s",__FUNCTION__, e.what());
	}
	return BattleAction::makeDefend(stack);
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
		logAi->warn("Warning: already standing on neighbouring tile!");
		//We shouldn't even be here...
		return BattleAction::makeDefend(stack);
	}
	vstd::erase_if(destNeighbours, [&](BattleHex hex){ return !reachability.accessibility.accessible(hex, stack); });
	if(!avHexes.size() || !destNeighbours.size()) //we are blocked or dest is blocked
	{
		return BattleAction::makeDefend(stack);
	}
	if(stack->hasBonusOfType(Bonus::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto distToDestNeighbour = [&](BattleHex hex) -> int
		{
			auto nearestNeighbourToHex = vstd::minElementByFun(destNeighbours, [&](BattleHex a)
			{return BattleHex::getDistance(a, hex);});
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

void CBattleAI::attemptCastingSpell()
{
	auto hero = cb->battleGetMyHero();
	if(!hero)
		return;

	if(!cb->battleCanCastSpell())
		return;

	LOGL("Casting spells sounds like fun. Let's see...");
	//Get all spells we can cast
	std::vector<const CSpell*> possibleSpells;
	vstd::copy_if(VLC->spellh->objects, std::back_inserter(possibleSpells), [this] (const CSpell *s) -> bool
	{
		auto problem = getCbc()->battleCanCastThisSpell(s);
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
		for(auto hex : getTargetsToConsider(spell, hero))
		{
			PossibleSpellcast ps = {spell, hex, 0};
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
			auto stacksSuffering = ps.spell->getAffectedStacks(cb.get(), ECastingMode::HERO_CASTING, hero, skillLevel, ps.dest);
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
			const int damageDiff = damageDealt - damageReceived * 10;
			LOGFL("Casting %s on hex %d would deal { %d %d } damage points among %d stacks.",
				  ps.spell->name % ps.dest % damageDealt % damageReceived % stacksSuffering.size());
			//TODO tactic effect too
			return damageDiff;
		}
		case TIMED_EFFECT:
		{
			auto stacksAffected = ps.spell->getAffectedStacks(cb.get(), ECastingMode::HERO_CASTING, hero, skillLevel, ps.dest);
			if(stacksAffected.empty())
				return -1;
			int totalGain = 0;
			for(const CStack * sta : stacksAffected)
			{
				StackWithBonuses swb;
				swb.stack = sta;
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
					  ps.spell->name % sta->nodeName() % (gain) % (oldValue) % (newValue));
				totalGain += gain;
			}

			LOGFL("Total gain of cast %s at hex %d is %d", ps.spell->name % (ps.dest.hex) % (totalGain));
			return totalGain;
		}
		default:
			assert(0);
			return 0;
		}
	};

	for(PossibleSpellcast & psc : possibleCasts)
		psc.value = evaluateSpellcast(psc);
	auto pscValue = [] (const PossibleSpellcast &ps) -> int
	{
		return ps.value;
	};
	auto castToPerform = *vstd::maxElementByFun(possibleCasts, pscValue);
	LOGFL("Best spell is %s. Will cast.", castToPerform.spell->name);
	BattleAction spellcast;
	spellcast.actionType = Battle::HERO_SPELL;
	spellcast.additionalInfo = castToPerform.spell->id;
	spellcast.destinationTile = castToPerform.dest;
	spellcast.side = side;
	spellcast.stackNumber = (!side) ? -1 : -2;
	cb->battleMakeAction(&spellcast);
}

std::vector<BattleHex> CBattleAI::getTargetsToConsider(const CSpell * spell, const ISpellCaster * caster) const
{
	const CSpell::TargetInfo targetInfo(spell, caster->getSpellSchoolLevel(spell));
	std::vector<BattleHex> ret;
	if(targetInfo.massive || targetInfo.type == CSpell::NO_TARGET)
	{
		ret.push_back(BattleHex());
	}
	else
	{
		switch(targetInfo.type)
		{
		case CSpell::CREATURE:
		{
			for(const CStack * stack : getCbc()->battleAliveStacks())
			{
				bool immune = ESpellCastProblem::OK != spell->isImmuneByStack(caster, stack);
				bool casterStack = stack->owner == caster->getOwner();

				if(!immune)
					switch (spell->positiveness)
					{
					case CSpell::POSITIVE:
						if(casterStack || targetInfo.smart)
							ret.push_back(stack->position);
						break;
					case CSpell::NEUTRAL:
						ret.push_back(stack->position);
						break;
					case CSpell::NEGATIVE:
						if(!casterStack || targetInfo.smart)
							ret.push_back(stack->position);
						break;
					}
			}
		}
			break;
		case CSpell::LOCATION:
		{
			for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
				if(BattleHex(i).isAvailable())
					ret.push_back(i);
		}
			break;

		default:
			break;
		}
	}
	return ret;
}

int CBattleAI::distToNearestNeighbour(BattleHex hex, const ReachabilityInfo::TDistances &dists, BattleHex *chosenHex)
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

void CBattleAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side)
{
	print("battleStart called");
	side = Side;
}

bool CBattleAI::isCloser(const EnemyInfo &ei1, const EnemyInfo &ei2, const ReachabilityInfo::TDistances &dists)
{
	return distToNearestNeighbour(ei1.s->position, dists) < distToNearestNeighbour(ei2.s->position, dists);
}

void CBattleAI::print(const std::string &text) const
{
	logAi->trace("CBattleAI [%p]: %s", this, text);
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



