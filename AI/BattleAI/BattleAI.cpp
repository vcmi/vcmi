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
#include "../../lib/CStopWatch.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/battle/BattleStateInfoForRetreat.h"
#include "../../lib/CStack.h" // TODO: remove
                              // Eventually only IBattleInfoCallback and battle::Unit should be used,
                              // CUnitState should be private and CStack should be removed completely

#define LOGL(text) print(text)
#define LOGFL(text, formattingEl) print(boost::str(boost::format(text) % formattingEl))

enum class SpellTypes
{
	ADVENTURE, BATTLE, OTHER
};

SpellTypes spellType(const CSpell * spell)
{
	if(!spell->isCombat() || spell->isCreatureAbility())
		return SpellTypes::OTHER;

	if(spell->isOffensive() || spell->hasEffects() || spell->hasBattleEffects())
		return SpellTypes::BATTLE;

	return SpellTypes::OTHER;
}

std::vector<BattleHex> CBattleAI::getBrokenWallMoatHexes() const
{
	std::vector<BattleHex> result;

	for(int wallPart = EWallPart::BOTTOM_WALL; wallPart < EWallPart::UPPER_WALL; wallPart++)
	{
		auto state = cb->battleGetWallState(wallPart);

		if(state != EWallState::DESTROYED)
			continue;

		auto wallHex = cb->wallPartToBattleHex((EWallPart::EWallPart)wallPart);
		auto moatHex = wallHex.cloneInDirection(BattleHex::LEFT);

		result.push_back(moatHex);
	}

	return result;
}

CBattleAI::CBattleAI()
	: side(-1),
	wasWaitingForRealize(false),
	wasUnlockingGs(false)
{
}

CBattleAI::~CBattleAI()
{
	if(cb)
	{
		//Restore previous state of CB - it may be shared with the main AI (like VCAI)
		cb->waitTillRealize = wasWaitingForRealize;
		cb->unlockGsWhenWaiting = wasUnlockingGs;
	}
}

void CBattleAI::init(std::shared_ptr<Environment> ENV, std::shared_ptr<CBattleCallback> CB)
{
	setCbc(CB);
	env = ENV;
	cb = CB;
	playerID = *CB->getPlayerID(); //TODO should be sth in callback
	wasWaitingForRealize = CB->waitTillRealize;
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
				if(auto woundHp = stack->MaxHealth() - stack->getFirstHPleft())
					woundHpToStack[woundHp] = stack;
			if(woundHpToStack.empty())
				return BattleAction::makeDefend(stack);
			else
				return BattleAction::makeHeal(stack, woundHpToStack.rbegin()->second); //last element of the woundHpToStack is the most wounded stack
		}

		attemptCastingSpell();

		if(auto ret = cb->battleIsFinished())
		{
			//spellcast may finish battle
			//send special preudo-action
			BattleAction cancel;
			cancel.actionType = EActionType::CANCEL;
			return cancel;
		}

		if(auto action = considerFleeingOrSurrendering())
			return *action;
		//best action is from effective owner point if view, we are effective owner as we received "activeStack"


		//evaluate casting spell for spellcasting stack
		boost::optional<PossibleSpellcast> bestSpellcast(boost::none);
		//TODO: faerie dragon type spell should be selected by server
		SpellID creatureSpellToCast = cb->battleGetRandomStackSpell(CRandomGenerator::getDefault(), stack, CBattleInfoCallback::RANDOM_AIMED);
		if(stack->hasBonusOfType(Bonus::SPELLCASTER) && stack->canCast() && creatureSpellToCast != SpellID::NONE)
		{
			const CSpell * spell = creatureSpellToCast.toSpell();

			if(spell->canBeCast(getCbc().get(), spells::Mode::CREATURE_ACTIVE, stack))
			{
				std::vector<PossibleSpellcast> possibleCasts;
				spells::BattleCast temp(getCbc().get(), stack, spells::Mode::CREATURE_ACTIVE, spell);
				for(auto & target : temp.findPotentialTargets())
				{
					PossibleSpellcast ps;
					ps.dest = target;
					ps.spell = spell;
					evaluateCreatureSpellcast(stack, ps);
					possibleCasts.push_back(ps);
				}

				std::sort(possibleCasts.begin(), possibleCasts.end(), [&](const PossibleSpellcast & lhs, const PossibleSpellcast & rhs) { return lhs.value > rhs.value; });
				if(!possibleCasts.empty() && possibleCasts.front().value > 0)
				{
					bestSpellcast = boost::optional<PossibleSpellcast>(possibleCasts.front());
				}
			}
		}

		HypotheticBattle hb(env.get(), cb);

		PotentialTargets targets(stack, &hb);

		if(!targets.possibleAttacks.empty())
		{
			AttackPossibility bestAttack = targets.bestAction();

			//TODO: consider more complex spellcast evaluation, f.e. because "re-retaliation" during enemy move in same turn for melee attack etc.
			if(bestSpellcast.is_initialized() && bestSpellcast->value > bestAttack.damageDiff())
				return BattleAction::makeCreatureSpellcast(stack, bestSpellcast->dest, bestSpellcast->spell->id);
			else if(bestAttack.attack.shooting)
			{
				auto &target = bestAttack;
				logAi->debug("BattleAI: %s -> %s x %d, shot, from %d curpos %d dist %d speed %d: %lld %lld %lld",
					target.attackerState->unitType()->identifier,
					target.affectedUnits[0]->unitType()->identifier,
					(int)target.affectedUnits.size(), (int)target.from, (int)bestAttack.attack.attacker->getPosition().hex,
					bestAttack.attack.chargedFields, bestAttack.attack.attacker->Speed(0, true),
					target.damageDealt, target.damageReceived, target.attackValue()
				);

				return BattleAction::makeShotAttack(stack, bestAttack.attack.defender);
			}
			else
			{
				auto &target = bestAttack;
				logAi->debug("BattleAI: %s -> %s x %d, mellee, from %d curpos %d dist %d speed %d: %lld %lld %lld",
					target.attackerState->unitType()->identifier,
					target.affectedUnits[0]->unitType()->identifier,
					(int)target.affectedUnits.size(), (int)target.from, (int)bestAttack.attack.attacker->getPosition().hex,
					bestAttack.attack.chargedFields, bestAttack.attack.attacker->Speed(0, true),
					target.damageDealt, target.damageReceived, target.attackValue()
				);

				return BattleAction::makeMeleeAttack(stack,	bestAttack.attack.defender->getPosition(), bestAttack.from);
		}
		}
		else if(bestSpellcast.is_initialized())
		{
			return BattleAction::makeCreatureSpellcast(stack, bestSpellcast->dest, bestSpellcast->spell->id);
		}
		else
		{
			if(stack->waited())
			{
				//ThreatMap threatsToUs(stack); // These lines may be usefull but they are't used in the code.
				auto dists = cb->getReachability(stack);
				if(!targets.unreachableEnemies.empty())
				{
					auto closestEnemy = vstd::minElementByFun(targets.unreachableEnemies, [&](const battle::Unit * enemy) -> int
					{
						return dists.distToNearestNeighbour(stack, enemy);
					});

					if(dists.distToNearestNeighbour(stack, *closestEnemy) < GameConstants::BFIELD_SIZE)
					{
						return goTowardsNearest(stack, (*closestEnemy)->getAttackableHexes(stack));
					}
				}
			}
			else
			{
				return BattleAction::makeWait(stack);
			}
		}

		if(!stack->hasBonusOfType(Bonus::FLYING)
			&& stack->unitSide() == BattleSide::ATTACKER
			&& cb->battleGetSiegeLevel() >= CGTownInstance::CITADEL)
		{
			auto brokenWallMoat = getBrokenWallMoatHexes();

			if(brokenWallMoat.size())
			{
				if(stack->doubleWide() && vstd::contains(brokenWallMoat, stack->getPosition()))
					return BattleAction::makeMove(stack, stack->getPosition().cloneInDirection(BattleHex::RIGHT));
				else
					return goTowardsNearest(stack, brokenWallMoat);
	}
		}
	}
	catch(boost::thread_interrupted &)
	{
		throw;
	}
	catch(std::exception &e)
	{
		logAi->error("Exception occurred in %s %s",__FUNCTION__, e.what());
	}

	return BattleAction::makeDefend(stack);
}

BattleAction CBattleAI::goTowardsNearest(const CStack * stack, std::vector<BattleHex> hexes) const
{
	auto reachability = cb->getReachability(stack);
	auto avHexes = cb->battleGetAvailableHexes(reachability, stack);

	if(!avHexes.size() || !hexes.size()) //we are blocked or dest is blocked
	{
		return BattleAction::makeDefend(stack);
	}

	std::sort(hexes.begin(), hexes.end(), [&](BattleHex h1, BattleHex h2) -> bool
	{
		return reachability.distances[h1] < reachability.distances[h2];
	});

	for(auto hex : hexes)
	{
		if(vstd::contains(avHexes, hex))
			return BattleAction::makeMove(stack, hex);

		if(stack->coversPos(hex))
		{
		logAi->warn("Warning: already standing on neighbouring tile!");
		//We shouldn't even be here...
		return BattleAction::makeDefend(stack);
	}
	}

	BattleHex bestNeighbor = hexes.front();

	if(reachability.distances[bestNeighbor] > GameConstants::BFIELD_SIZE)
	{
		return BattleAction::makeDefend(stack);
	}

	if(stack->hasBonusOfType(Bonus::FLYING))
	{
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto nearestAvailableHex = vstd::minElementByFun(avHexes, [&](BattleHex hex) -> int
		{
			return BattleHex::getDistance(bestNeighbor, hex);
		});

		return BattleAction::makeMove(stack, *nearestAvailableHex);
	}
	else
	{
		BattleHex currentDest = bestNeighbor;
		while(1)
		{
			if(!currentDest.isValid())
			{
				logAi->error("CBattleAI::goTowards: internal error");
				return BattleAction::makeDefend(stack);
			}

			if(vstd::contains(avHexes, currentDest))
				return BattleAction::makeMove(stack, currentDest);

			currentDest = reachability.predecessors[currentDest];
		}
	}
}

BattleAction CBattleAI::useCatapult(const CStack * stack)
{
	BattleAction attack;
	BattleHex targetHex = BattleHex::INVALID;

	if(cb->battleGetGateState() == EGateState::CLOSED)
	{
		targetHex = cb->wallPartToBattleHex(EWallPart::GATE);
	}
	else
	{
		EWallPart::EWallPart wallParts[] = {
			EWallPart::KEEP,
			EWallPart::BOTTOM_TOWER,
			EWallPart::UPPER_TOWER,
			EWallPart::BELOW_GATE,
			EWallPart::OVER_GATE,
			EWallPart::BOTTOM_WALL,
			EWallPart::UPPER_WALL
		};

		for(auto wallPart : wallParts)
		{
			auto wallState = cb->battleGetWallState(wallPart);

			if(wallState == EWallState::INTACT || wallState == EWallState::DAMAGED)
			{
				targetHex = cb->wallPartToBattleHex(wallPart);

				break;
			}
		}
	}

	if(!targetHex.isValid())
	{
		return BattleAction::makeDefend(stack);
	}

	attack.aimToHex(targetHex);
	attack.actionType = EActionType::CATAPULT;
	attack.side = side;
	attack.stackNumber = stack->ID;

	return attack;
}

void CBattleAI::attemptCastingSpell()
{
	auto hero = cb->battleGetMyHero();
	if(!hero)
		return;

	if(cb->battleCanCastSpell(hero, spells::Mode::HERO) != ESpellCastProblem::OK)
		return;

	LOGL("Casting spells sounds like fun. Let's see...");
	//Get all spells we can cast
	std::vector<const CSpell*> possibleSpells;
	vstd::copy_if(VLC->spellh->objects, std::back_inserter(possibleSpells), [hero, this](const CSpell *s) -> bool
	{
		return s->canBeCast(cb.get(), spells::Mode::HERO, hero);
	});
	LOGFL("I can cast %d spells.", possibleSpells.size());

	vstd::erase_if(possibleSpells, [](const CSpell *s)
	{
		return spellType(s) != SpellTypes::BATTLE;
	});

	LOGFL("I know how %d of them works.", possibleSpells.size());

	//Get possible spell-target pairs
	std::vector<PossibleSpellcast> possibleCasts;
	for(auto spell : possibleSpells)
	{
		spells::BattleCast temp(cb.get(), hero, spells::Mode::HERO, spell);

		for(auto & target : temp.findPotentialTargets())
		{
			PossibleSpellcast ps;
			ps.dest = target;
			ps.spell = spell;
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return;

	using ValueMap = PossibleSpellcast::ValueMap;

	auto evaluateQueue = [&](ValueMap & values, const std::vector<battle::Units> & queue, HypotheticBattle * state, size_t minTurnSpan, bool * enemyHadTurnOut) -> bool
	{
		bool firstRound = true;
		bool enemyHadTurn = false;
		size_t ourTurnSpan = 0;

		bool stop = false;

		for(auto & round : queue)
		{
			if(!firstRound)
				state->nextRound(0);//todo: set actual value?
			for(auto unit : round)
			{
				if(!vstd::contains(values, unit->unitId()))
					values[unit->unitId()] = 0;

				if(!unit->alive())
					continue;

				if(state->battleGetOwner(unit) != playerID)
				{
					enemyHadTurn = true;

					if(!firstRound || state->battleCastSpells(unit->unitSide()) == 0)
					{
						//enemy could counter our spell at this point
						//anyway, we do not know what enemy will do
						//just stop evaluation
						stop = true;
						break;
					}
				}
				else if(!enemyHadTurn)
				{
					ourTurnSpan++;
				}

				state->nextTurn(unit->unitId());

				PotentialTargets pt(unit, state);

				if(!pt.possibleAttacks.empty())
				{
					AttackPossibility ap = pt.bestAction();

					auto swb = state->getForUpdate(unit->unitId());
					*swb = *ap.attackerState;

					if(ap.damageDealt > 0)
						swb->removeUnitBonus(Bonus::UntilAttack);
					if(ap.damageReceived > 0)
						swb->removeUnitBonus(Bonus::UntilBeingAttacked);

					for(auto affected : ap.affectedUnits)
					{
						swb = state->getForUpdate(affected->unitId());
						*swb = *affected;

						if(ap.damageDealt > 0)
							swb->removeUnitBonus(Bonus::UntilBeingAttacked);
						if(ap.damageReceived > 0 && ap.attack.defender->unitId() == affected->unitId())
							swb->removeUnitBonus(Bonus::UntilAttack);
					}
				}

				auto bav = pt.bestActionValue();

				//best action is from effective owner`s point if view, we need to convert to our point if view
				if(state->battleGetOwner(unit) != playerID)
					bav = -bav;
				values[unit->unitId()] += bav;
			}

			firstRound = false;

			if(stop)
				break;
		}

		if(enemyHadTurnOut)
			*enemyHadTurnOut = enemyHadTurn;

		return ourTurnSpan >= minTurnSpan;
	};

	ValueMap valueOfStack;
	ValueMap healthOfStack;

	TStacks all = cb->battleGetAllStacks(false);

	size_t ourRemainingTurns = 0;

	for(auto unit : all)
	{
		healthOfStack[unit->unitId()] = unit->getAvailableHealth();
		valueOfStack[unit->unitId()] = 0;

		if(cb->battleGetOwner(unit) == playerID && unit->canMove() && !unit->moved())
			ourRemainingTurns++;
	}

	LOGFL("I have %d turns left in this round", ourRemainingTurns);

	const bool castNow = ourRemainingTurns <= 1;

	if(castNow)
		print("I should try to cast a spell now");
	else
		print("I could wait better moment to cast a spell");

	auto amount = all.size();

	std::vector<battle::Units> turnOrder;

	cb->battleGetTurnOrder(turnOrder, amount, 2); //no more than 1 turn after current, each unit at least once

	{
		bool enemyHadTurn = false;

		HypotheticBattle state(env.get(), cb);

		evaluateQueue(valueOfStack, turnOrder, &state, 0, &enemyHadTurn);

		if(!enemyHadTurn)
		{
			auto battleIsFinishedOpt = state.battleIsFinished();

			if(battleIsFinishedOpt)
			{
				print("No need to cast a spell. Battle will finish soon.");
				return;
			}
		}
	}

	struct ScriptsCache
	{
		//todo: re-implement scripts context cache
	};

	auto evaluateSpellcast = [&] (PossibleSpellcast * ps, std::shared_ptr<ScriptsCache>)
	{
		HypotheticBattle state(env.get(), cb);

		spells::BattleCast cast(&state, hero, spells::Mode::HERO, ps->spell);
		cast.castEval(state.getServerCallback(), ps->dest);
		ValueMap newHealthOfStack;
		ValueMap newValueOfStack;

		size_t ourUnits = 0;

		for(auto unit : all)
		{
			auto unitId = unit->unitId();
			auto localUnit = state.battleGetUnitByID(unitId);

			newHealthOfStack[unitId] = localUnit->getAvailableHealth();
			newValueOfStack[unitId] = 0;

			if(state.battleGetOwner(localUnit) == playerID && localUnit->alive() && localUnit->willMove())
				ourUnits++;
		}

		size_t minTurnSpan = ourUnits/3; //todo: tweak this

		std::vector<battle::Units> newTurnOrder;

		state.battleGetTurnOrder(newTurnOrder, amount, 2);

		const bool turnSpanOK = evaluateQueue(newValueOfStack, newTurnOrder, &state, minTurnSpan, nullptr);

		if(turnSpanOK || castNow)
		{
			int64_t totalGain = 0;

			for(auto unit : all)
			{
				auto unitId = unit->unitId();
				auto localUnit = state.battleGetUnitByID(unitId);

				auto newValue = getValOr(newValueOfStack, unitId, 0);
				auto oldValue = getValOr(valueOfStack, unitId, 0);

				auto healthDiff = newHealthOfStack[unitId] - healthOfStack[unitId];

				if(localUnit->unitOwner() != playerID)
					healthDiff = -healthDiff;

				if(healthDiff < 0)
				{
					ps->value = -1;
					return; //do not damage own units at all
				}

				totalGain += (newValue - oldValue + healthDiff);
			}

			ps->value = totalGain;
		}
		else
		{
			ps->value = -1;
		}
	};

	using EvalRunner = ThreadPool<ScriptsCache>;

	EvalRunner::Tasks tasks;

	for(PossibleSpellcast & psc : possibleCasts)
		tasks.push_back(std::bind(evaluateSpellcast, &psc, _1));

	uint32_t threadCount = boost::thread::hardware_concurrency();

	if(threadCount == 0)
	{
		logGlobal->warn("No information of CPU cores available");
		threadCount = 1;
	}

	CStopWatch timer;

	std::vector<std::shared_ptr<ScriptsCache>> scriptsPool;

	for(uint32_t idx = 0; idx < threadCount; idx++)
	{
		scriptsPool.emplace_back();
	}

	EvalRunner runner(&tasks, scriptsPool);
	runner.run();

	LOGFL("Evaluation took %d ms", timer.getDiff());

	auto pscValue = [](const PossibleSpellcast &ps) -> int64_t
	{
		return ps.value;
	};
	auto castToPerform = *vstd::maxElementByFun(possibleCasts, pscValue);

	if(castToPerform.value > 0)
	{
		LOGFL("Best spell is %s (value %d). Will cast.", castToPerform.spell->name % castToPerform.value);
		BattleAction spellcast;
		spellcast.actionType = EActionType::HERO_SPELL;
		spellcast.actionSubtype = castToPerform.spell->id;
		spellcast.setTarget(castToPerform.dest);
		spellcast.side = side;
		spellcast.stackNumber = (!side) ? -1 : -2;
		cb->battleMakeAction(&spellcast);
	}
	else
	{
		LOGFL("Best spell is %s. But it is actually useless (value %d).", castToPerform.spell->name % castToPerform.value);
	}
}

//Below method works only for offensive spells
void CBattleAI::evaluateCreatureSpellcast(const CStack * stack, PossibleSpellcast & ps)
{
	using ValueMap = PossibleSpellcast::ValueMap;

	RNGStub rngStub;
	HypotheticBattle state(env.get(), cb);
	TStacks all = cb->battleGetAllStacks(false);

	ValueMap healthOfStack;
	ValueMap newHealthOfStack;

	for(auto unit : all)
	{
		healthOfStack[unit->unitId()] = unit->getAvailableHealth();
	}

	spells::BattleCast cast(&state, stack, spells::Mode::CREATURE_ACTIVE, ps.spell);
	cast.castEval(state.getServerCallback(), ps.dest);

	for(auto unit : all)
	{
		auto unitId = unit->unitId();
		auto localUnit = state.battleGetUnitByID(unitId);
		newHealthOfStack[unitId] = localUnit->getAvailableHealth();
	}

	int64_t totalGain = 0;

	for(auto unit : all)
	{
		auto unitId = unit->unitId();
		auto localUnit = state.battleGetUnitByID(unitId);

		auto healthDiff = newHealthOfStack[unitId] - healthOfStack[unitId];

		if(localUnit->unitOwner() != getCbc()->getPlayerID())
			healthDiff = -healthDiff;

		if(healthDiff < 0)
		{
			ps.value = -1;
			return; //do not damage own units at all
		}

		totalGain += healthDiff;
	}

	ps.value = totalGain;
}

void CBattleAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool Side)
{
	LOG_TRACE(logAi);
	side = Side;
}

void CBattleAI::print(const std::string &text) const
{
	logAi->trace("%s Battle AI[%p]: %s", playerID.getStr(), this, text);
}

boost::optional<BattleAction> CBattleAI::considerFleeingOrSurrendering()
{
	BattleStateInfoForRetreat bs;

	bs.canFlee = cb->battleCanFlee();
	bs.canSurrender = cb->battleCanSurrender(playerID);
	bs.ourSide = cb->battleGetMySide();
	bs.ourHero = cb->battleGetMyHero(); 
	bs.enemyHero = nullptr;

	for(auto stack : cb->battleGetAllStacks(false))
	{
		if(stack->alive())
		{
			if(stack->side == bs.ourSide)
				bs.ourStacks.push_back(stack);
			else
			{
				bs.enemyStacks.push_back(stack);
				bs.enemyHero = cb->battleGetOwnerHero(stack);
			}
		}
	}

	if(!bs.canFlee || !bs.canSurrender)
	{
		return boost::none;
	}

	return cb->makeSurrenderRetreatDecision(bs);
}



