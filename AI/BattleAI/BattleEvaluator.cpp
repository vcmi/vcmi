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
#include "BattleEvaluator.h"
#include "BattleExchangeVariant.h"

#include "StackWithBonuses.h"
#include "EnemyInfo.h"
#include "tbb/parallel_for.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/entities/building/TownFortifications.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/battle/BattleStateInfoForRetreat.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/CRandomGenerator.h"


// TODO: remove
// Eventually only IBattleInfoCallback and battle::Unit should be used,
// CUnitState should be private and CStack should be removed completely
#include "../../lib/CStack.h"

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

BattleEvaluator::BattleEvaluator(
	std::shared_ptr<Environment> env,
	std::shared_ptr<CBattleCallback> cb,
	const battle::Unit * activeStack,
	PlayerColor playerID,
	BattleID battleID,
	BattleSide side,
	float strengthRatio,
	int simulationTurnsCount)
	:scoreEvaluator(cb->getBattle(battleID), env, strengthRatio, simulationTurnsCount),
	cachedAttack(), playerID(playerID), side(side), env(env),
	cb(cb), strengthRatio(strengthRatio), battleID(battleID), simulationTurnsCount(simulationTurnsCount)
{
	hb = std::make_shared<HypotheticBattle>(env.get(), cb->getBattle(battleID));
	damageCache.buildDamageCache(hb, side);

	targets = std::make_unique<PotentialTargets>(activeStack, damageCache, hb);
}

BattleEvaluator::BattleEvaluator(
	std::shared_ptr<Environment> env,
	std::shared_ptr<CBattleCallback> cb,
	std::shared_ptr<HypotheticBattle> hb,
	DamageCache & damageCache,
	const battle::Unit * activeStack,
	PlayerColor playerID,
	BattleID battleID,
	BattleSide side,
	float strengthRatio,
	int simulationTurnsCount)
	:scoreEvaluator(cb->getBattle(battleID), env, strengthRatio, simulationTurnsCount),
	cachedAttack(), playerID(playerID), side(side), env(env), cb(cb), hb(hb),
	damageCache(damageCache), strengthRatio(strengthRatio), battleID(battleID), simulationTurnsCount(simulationTurnsCount)
{
	targets = std::make_unique<PotentialTargets>(activeStack, damageCache, hb);
}

std::vector<BattleHex> BattleEvaluator::getBrokenWallMoatHexes() const
{
	std::vector<BattleHex> result;

	for(EWallPart wallPart : { EWallPart::BOTTOM_WALL, EWallPart::BELOW_GATE, EWallPart::OVER_GATE, EWallPart::UPPER_WALL })
	{
		auto state = cb->getBattle(battleID)->battleGetWallState(wallPart);

		if(state != EWallState::DESTROYED)
			continue;

		auto wallHex = cb->getBattle(battleID)->wallPartToBattleHex(wallPart);
		auto moatHex = wallHex.cloneInDirection(BattleHex::LEFT);

		result.push_back(moatHex);

		moatHex = moatHex.cloneInDirection(BattleHex::LEFT);
		auto obstaclesSecondRow = cb->getBattle(battleID)->battleGetAllObstaclesOnPos(moatHex, false);

		for(auto obstacle : obstaclesSecondRow)
		{
			if(obstacle->obstacleType == CObstacleInstance::EObstacleType::MOAT)
			{
				result.push_back(moatHex);
				break;
			}
		}
	}

	return result;
}

std::optional<PossibleSpellcast> BattleEvaluator::findBestCreatureSpell(const CStack *stack)
{
	//TODO: faerie dragon type spell should be selected by server
	SpellID creatureSpellToCast = cb->getBattle(battleID)->getRandomCastedSpell(CRandomGenerator::getDefault(), stack);

	if(stack->canCast() && creatureSpellToCast != SpellID::NONE)
	{
		const CSpell * spell = creatureSpellToCast.toSpell();

		if(spell->canBeCast(cb->getBattle(battleID).get(), spells::Mode::CREATURE_ACTIVE, stack))
		{
			std::vector<PossibleSpellcast> possibleCasts;
			spells::BattleCast temp(cb->getBattle(battleID).get(), stack, spells::Mode::CREATURE_ACTIVE, spell);
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
				return possibleCasts.front();
			}
		}
	}
	return std::nullopt;
}

BattleAction BattleEvaluator::selectStackAction(const CStack * stack)
{
#if BATTLE_TRACE_LEVEL >= 1
	logAi->trace("Select stack action");
#endif
	//evaluate casting spell for spellcasting stack
	std::optional<PossibleSpellcast> bestSpellcast = findBestCreatureSpell(stack);

	auto moveTarget = scoreEvaluator.findMoveTowardsUnreachable(stack, *targets, damageCache, hb);
	float score = EvaluationResult::INEFFECTIVE_SCORE;

	if(targets->possibleAttacks.empty() && bestSpellcast.has_value())
	{
		activeActionMade = true;
		return BattleAction::makeCreatureSpellcast(stack, bestSpellcast->dest, bestSpellcast->spell->id);
	}

	if(!targets->possibleAttacks.empty())
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace("Evaluating attack for %s", stack->getDescription());
#endif

		auto evaluationResult = scoreEvaluator.findBestTarget(stack, *targets, damageCache, hb);
		auto & bestAttack = evaluationResult.bestAttack;

		cachedAttack.ap = bestAttack;
		cachedAttack.score = evaluationResult.score;
		cachedAttack.turn = 0;
		cachedAttack.waited = evaluationResult.wait;

		//TODO: consider more complex spellcast evaluation, f.e. because "re-retaliation" during enemy move in same turn for melee attack etc.
		if(bestSpellcast.has_value() && bestSpellcast->value > bestAttack.damageDiff())
		{
			// return because spellcast value is damage dealt and score is dps reduce
			activeActionMade = true;
			return BattleAction::makeCreatureSpellcast(stack, bestSpellcast->dest, bestSpellcast->spell->id);
		}

		if(evaluationResult.score > score)
		{
			score = evaluationResult.score;

			logAi->debug("BattleAI: %s -> %s x %d, from %d curpos %d dist %d speed %d: +%2f -%2f = %2f",
				bestAttack.attackerState->unitType()->getJsonKey(),
				bestAttack.affectedUnits[0]->unitType()->getJsonKey(),
				bestAttack.affectedUnits[0]->getCount(),
				(int)bestAttack.from,
				(int)bestAttack.attack.attacker->getPosition().hex,
				bestAttack.attack.chargeDistance,
				bestAttack.attack.attacker->getMovementRange(0),
				bestAttack.defenderDamageReduce,
				bestAttack.attackerDamageReduce,
				score
			);

			if (moveTarget.score <= score)
			{
				if(evaluationResult.wait)
				{
					return BattleAction::makeWait(stack);
				}
				else if(bestAttack.attack.shooting)
				{
					activeActionMade = true;
					return BattleAction::makeShotAttack(stack, bestAttack.attack.defender);
				}
				else
				{
					if(bestAttack.collateralDamageReduce
						&& bestAttack.collateralDamageReduce >= bestAttack.defenderDamageReduce / 2
						&& score < 0)
					{
						return BattleAction::makeDefend(stack);
					}

					auto enemyMellee = hb->getUnitsIf([this](const battle::Unit * u) -> bool
						{
							return u->unitSide() == BattleSide::ATTACKER && !hb->battleCanShoot(u);
						});

					bool isTargetOutsideFort = bestAttack.dest.getY() < GameConstants::BFIELD_WIDTH - 4;
					bool siegeDefense = stack->unitSide() == BattleSide::DEFENDER
						&& !bestAttack.attack.shooting
						&& hb->battleGetFortifications().hasMoat
						&& !enemyMellee.empty()
						&& isTargetOutsideFort;

					if(siegeDefense)
					{
						logAi->trace("Evaluating exchange at %d self-defense", stack->getPosition().hex);

						BattleAttackInfo bai(stack, stack, 0, false);
						AttackPossibility apDefend(stack->getPosition(), stack->getPosition(), bai);

						float defenseValue = scoreEvaluator.evaluateExchange(apDefend, 0, *targets, damageCache, hb);

						if((defenseValue > score && score <= 0) || (defenseValue > 2 * score && score > 0))
						{
							return BattleAction::makeDefend(stack);
						}
					}
					
					activeActionMade = true;
					return BattleAction::makeMeleeAttack(stack, bestAttack.attack.defenderPos, bestAttack.from);
				}
			}
		}
	}

	//ThreatMap threatsToUs(stack); // These lines may be useful but they are't used in the code.
	if(moveTarget.score > score)
	{
		score = moveTarget.score;
		cachedAttack.ap = moveTarget.cachedAttack;
		cachedAttack.score = score;
		cachedAttack.turn = moveTarget.turnsToRich;

		if(stack->waited())
		{
			logAi->debug(
				"Moving %s towards hex %s[%d], score: %2f",
				stack->getDescription(),
				moveTarget.cachedAttack->attack.defender->getDescription(),
				moveTarget.cachedAttack->attack.defender->getPosition().hex,
				moveTarget.score);

			return goTowardsNearest(stack, moveTarget.positions, *targets);
		}
		else
		{
			cachedAttack.waited = true;

			return BattleAction::makeWait(stack);
		}
	}

	if(score <= EvaluationResult::INEFFECTIVE_SCORE
		&& !stack->hasBonusOfType(BonusType::FLYING)
		&& stack->unitSide() == BattleSide::ATTACKER
	   && cb->getBattle(battleID)->battleGetFortifications().hasMoat)
	{
		auto brokenWallMoat = getBrokenWallMoatHexes();

		if(brokenWallMoat.size())
		{
			activeActionMade = true;

			if(stack->doubleWide() && vstd::contains(brokenWallMoat, stack->getPosition()))
				return BattleAction::makeMove(stack, stack->getPosition().cloneInDirection(BattleHex::RIGHT));
			else
				return goTowardsNearest(stack, brokenWallMoat, *targets);
		}
	}

	return stack->waited() ?  BattleAction::makeDefend(stack) : BattleAction::makeWait(stack);
}

uint64_t timeElapsed(std::chrono::time_point<std::chrono::high_resolution_clock> start)
{
	auto end = std::chrono::high_resolution_clock::now();

	return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

BattleAction BattleEvaluator::moveOrAttack(const CStack * stack, BattleHex hex, const PotentialTargets & targets)
{
	auto additionalScore = 0;
	std::optional<AttackPossibility> attackOnTheWay;

	for(auto & target : targets.possibleAttacks)
	{
		if(!target.attack.shooting && target.from == hex && target.attackValue() > additionalScore)
		{
			additionalScore = target.attackValue();
			attackOnTheWay = target;
		}
	}

	if(attackOnTheWay)
	{
		activeActionMade = true;
		return BattleAction::makeMeleeAttack(stack, attackOnTheWay->attack.defender->getPosition(), attackOnTheWay->from);
	}
	else
	{
		return BattleAction::makeMove(stack, hex);
	}
}

BattleAction BattleEvaluator::goTowardsNearest(const CStack * stack, std::vector<BattleHex> hexes, const PotentialTargets & targets)
{
	auto reachability = cb->getBattle(battleID)->getReachability(stack);
	auto avHexes = cb->getBattle(battleID)->battleGetAvailableHexes(reachability, stack, false);

	if(!avHexes.size() || !hexes.size()) //we are blocked or dest is blocked
	{
		return BattleAction::makeDefend(stack);
	}

	std::vector<BattleHex> targetHexes = hexes;

	vstd::erase_if(targetHexes, [](const BattleHex & hex) { return !hex.isValid(); });

	std::sort(targetHexes.begin(), targetHexes.end(), [&](BattleHex h1, BattleHex h2) -> bool
		{
			return reachability.distances[h1] < reachability.distances[h2];
		});

	BattleHex bestNeighbor = targetHexes.front();

	if(reachability.distances[bestNeighbor] > GameConstants::BFIELD_SIZE)
	{
		logAi->trace("No richable hexes.");
		return BattleAction::makeDefend(stack);
	}

	// this turn
	for(auto hex : targetHexes)
	{
		if(vstd::contains(avHexes, hex))
		{
			return moveOrAttack(stack, hex, targets);
		}

		if(stack->coversPos(hex))
		{
			logAi->warn("Warning: already standing on neighbouring hex!");
			//We shouldn't even be here...
			return BattleAction::makeDefend(stack);
		}
	}

	// not this turn
	scoreEvaluator.updateReachabilityMap(hb);

	if(stack->hasBonusOfType(BonusType::FLYING))
	{
		std::set<BattleHex> obstacleHexes;

		auto insertAffected = [](const CObstacleInstance & spellObst, std::set<BattleHex> obstacleHexes) {
			auto affectedHexes = spellObst.getAffectedTiles();
			obstacleHexes.insert(affectedHexes.cbegin(), affectedHexes.cend());
		};

		const auto & obstacles = hb->battleGetAllObstacles();

		for (const auto & obst: obstacles) {

			if(obst->triggersEffects())
			{
				auto triggerAbility =  VLC->spells()->getById(obst->getTrigger());
				auto triggerIsNegative = triggerAbility->isNegative() || triggerAbility->isDamage();

				if(triggerIsNegative)
					insertAffected(*obst, obstacleHexes);
			}
		}
		// Flying stack doesn't go hex by hex, so we can't backtrack using predecessors.
		// We just check all available hexes and pick the one closest to the target.
		auto nearestAvailableHex = vstd::minElementByFun(avHexes, [&](BattleHex hex) -> int
		{
			const int NEGATIVE_OBSTACLE_PENALTY = 100; // avoid landing on negative obstacle (moat, fire wall, etc)
			const int BLOCKED_STACK_PENALTY = 100; // avoid landing on moat

			auto distance = BattleHex::getDistance(bestNeighbor, hex);

			if(vstd::contains(obstacleHexes, hex))
				distance += NEGATIVE_OBSTACLE_PENALTY;

			return scoreEvaluator.checkPositionBlocksOurStacks(*hb, stack, hex) ? BLOCKED_STACK_PENALTY + distance : distance;
		});

		return moveOrAttack(stack, *nearestAvailableHex, targets);
	}
	else
	{
		BattleHex currentDest = bestNeighbor;
		while(true)
		{
			if(!currentDest.isValid())
			{
				return BattleAction::makeDefend(stack);
			}

			if(vstd::contains(avHexes, currentDest)
				&& !scoreEvaluator.checkPositionBlocksOurStacks(*hb, stack, currentDest))
			{
				return moveOrAttack(stack, currentDest, targets);
			}

			currentDest = reachability.predecessors[currentDest];
		}
	}
	
	logAi->error("We should either detect that hexes are unreachable or make a move!");
	return BattleAction::makeDefend(stack);
}

bool BattleEvaluator::canCastSpell()
{
	auto hero = cb->getBattle(battleID)->battleGetMyHero();
	if(!hero)
		return false;

	return cb->getBattle(battleID)->battleCanCastSpell(hero, spells::Mode::HERO) == ESpellCastProblem::OK;
}

bool BattleEvaluator::attemptCastingSpell(const CStack * activeStack)
{
	auto hero = cb->getBattle(battleID)->battleGetMyHero();
	if(!hero)
		return false;

	LOGL("Casting spells sounds like fun. Let's see...");
	//Get all spells we can cast
	std::vector<const CSpell*> possibleSpells;

	for (auto const & s : VLC->spellh->objects)
		if (s->canBeCast(cb->getBattle(battleID).get(), spells::Mode::HERO, hero))
			possibleSpells.push_back(s.get());

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
		spells::BattleCast temp(cb->getBattle(battleID).get(), hero, spells::Mode::HERO, spell);

		const bool FAST = true;

		for(auto & target : temp.findPotentialTargets(FAST))
		{
			PossibleSpellcast ps;
			ps.dest = target;
			ps.spell = spell;
			possibleCasts.push_back(ps);
		}
	}
	LOGFL("Found %d spell-target combinations.", possibleCasts.size());
	if(possibleCasts.empty())
		return false;

	using ValueMap = PossibleSpellcast::ValueMap;

	auto evaluateQueue = [&](ValueMap & values, const std::vector<battle::Units> & queue, std::shared_ptr<HypotheticBattle> state, size_t minTurnSpan, bool * enemyHadTurnOut) -> bool
	{
		bool firstRound = true;
		bool enemyHadTurn = false;
		size_t ourTurnSpan = 0;

		bool stop = false;

		for(auto & round : queue)
		{
			if(!firstRound)
				state->nextRound();
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

				PotentialTargets potentialTargets(unit, damageCache, state);

				if(!potentialTargets.possibleAttacks.empty())
				{
					AttackPossibility attackPossibility = potentialTargets.bestAction();

					auto stackWithBonuses = state->getForUpdate(unit->unitId());
					*stackWithBonuses = *attackPossibility.attackerState;

					if(attackPossibility.defenderDamageReduce > 0)
					{
						stackWithBonuses->removeUnitBonus(Bonus::UntilAttack);
						stackWithBonuses->removeUnitBonus(Bonus::UntilOwnAttack);
					}
					if(attackPossibility.attackerDamageReduce > 0)
						stackWithBonuses->removeUnitBonus(Bonus::UntilBeingAttacked);

					for(auto affected : attackPossibility.affectedUnits)
					{
						stackWithBonuses = state->getForUpdate(affected->unitId());
						*stackWithBonuses = *affected;

						if(attackPossibility.defenderDamageReduce > 0)
							stackWithBonuses->removeUnitBonus(Bonus::UntilBeingAttacked);
						if(attackPossibility.attackerDamageReduce > 0 && attackPossibility.attack.defender->unitId() == affected->unitId())
							stackWithBonuses->removeUnitBonus(Bonus::UntilAttack);
					}
				}

				auto bav = potentialTargets.bestActionValue();

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

	TStacks all = cb->getBattle(battleID)->battleGetAllStacks(false);

	size_t ourRemainingTurns = 0;

	for(auto unit : all)
	{
		healthOfStack[unit->unitId()] = unit->getAvailableHealth();
		valueOfStack[unit->unitId()] = 0;

		if(cb->getBattle(battleID)->battleGetOwner(unit) == playerID && unit->canMove() && !unit->moved())
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

	cb->getBattle(battleID)->battleGetTurnOrder(turnOrder, amount, 2); //no more than 1 turn after current, each unit at least once

	{
		bool enemyHadTurn = false;

		auto state = std::make_shared<HypotheticBattle>(env.get(), cb->getBattle(battleID));

		evaluateQueue(valueOfStack, turnOrder, state, 0, &enemyHadTurn);

		if(!enemyHadTurn)
		{
			auto battleIsFinishedOpt = state->battleIsFinished();

			if(battleIsFinishedOpt)
			{
				print("No need to cast a spell. Battle will finish soon.");
				return false;
			}
		}
	}

	CStopWatch timer;

#if BATTLE_TRACE_LEVEL >= 1
	tbb::blocked_range<size_t> r(0, possibleCasts.size());
#else
	tbb::parallel_for(tbb::blocked_range<size_t>(0, possibleCasts.size()), [&](const tbb::blocked_range<size_t> & r)
		{
#endif
			for(auto i = r.begin(); i != r.end(); i++)
			{
				auto & ps = possibleCasts[i];

#if BATTLE_TRACE_LEVEL >= 1
				if(ps.dest.empty())
					logAi->trace("Evaluating %s", ps.spell->getNameTranslated());
				else
				{
					auto psFirst = ps.dest.front();
					auto strWhere = psFirst.unitValue ? psFirst.unitValue->getDescription() : std::to_string(psFirst.hexValue.hex);

					logAi->trace("Evaluating %s at %s", ps.spell->getNameTranslated(), strWhere);
				}
#endif

				auto state = std::make_shared<HypotheticBattle>(env.get(), cb->getBattle(battleID));

				spells::BattleCast cast(state.get(), hero, spells::Mode::HERO, ps.spell);
				cast.castEval(state->getServerCallback(), ps.dest);

				auto allUnits = state->battleGetUnitsIf([](const battle::Unit * u) -> bool { return true; });

				auto needFullEval = vstd::contains_if(allUnits, [&](const battle::Unit * u) -> bool
					{
						auto original = cb->getBattle(battleID)->battleGetUnitByID(u->unitId());
						return  !original || u->getMovementRange() != original->getMovementRange();
					});

				DamageCache safeCopy = damageCache;
				DamageCache innerCache(&safeCopy);

				innerCache.buildDamageCache(state, side);

				if(cachedAttack.ap && cachedAttack.waited)
				{
					state->makeWait(activeStack);
				}

				if(needFullEval || !cachedAttack.ap)
				{
#if BATTLE_TRACE_LEVEL >= 1
					logAi->trace("Full evaluation is started due to stack speed affected.");
#endif

					PotentialTargets innerTargets(activeStack, innerCache, state);
					BattleExchangeEvaluator innerEvaluator(state, env, strengthRatio, simulationTurnsCount);

					innerEvaluator.updateReachabilityMap(state);

					auto moveTarget = innerEvaluator.findMoveTowardsUnreachable(activeStack, innerTargets, innerCache, state);

					if(!innerTargets.possibleAttacks.empty())
					{
						auto newStackAction = innerEvaluator.findBestTarget(activeStack, innerTargets, innerCache, state);

						ps.value = std::max(moveTarget.score, newStackAction.score);
					}
					else
					{
						ps.value = moveTarget.score;
					}
				}
				else
				{
					auto updatedAttacker = state->getForUpdate(cachedAttack.ap->attack.attacker->unitId());
					auto updatedDefender = state->getForUpdate(cachedAttack.ap->attack.defender->unitId());
					auto updatedBai = BattleAttackInfo(
						updatedAttacker.get(),
						updatedDefender.get(),
						cachedAttack.ap->attack.chargeDistance,
						cachedAttack.ap->attack.shooting);

					auto updatedAttack = AttackPossibility::evaluate(updatedBai, cachedAttack.ap->from, innerCache, state);

					ps.value = scoreEvaluator.evaluateExchange(updatedAttack, cachedAttack.turn, *targets, innerCache, state);
				}

				for(const auto & unit : allUnits)
				{
					if(!unit->isValidTarget(true))
						continue;

					auto newHealth = unit->getAvailableHealth();
					auto oldHealth = vstd::find_or(healthOfStack, unit->unitId(), 0); // old health value may not exist for newly summoned units

					if(oldHealth != newHealth)
					{
						auto damage = std::abs(oldHealth - newHealth);
						auto originalDefender = cb->getBattle(battleID)->battleGetUnitByID(unit->unitId());

						auto dpsReduce = AttackPossibility::calculateDamageReduce(
							nullptr,
							originalDefender && originalDefender->alive() ? originalDefender : unit,
							damage,
							innerCache,
							state);

						auto ourUnit = unit->unitSide() == side ? 1 : -1;
						auto goodEffect = newHealth > oldHealth ? 1 : -1;

						if(ourUnit * goodEffect == 1)
						{
							auto isMagical = state->getForUpdate(unit->unitId())->summoned
								|| unit->isClone()
								|| unit->isGhost();

							if(ourUnit && goodEffect && isMagical)
								continue;

							ps.value += dpsReduce * scoreEvaluator.getPositiveEffectMultiplier();
						}
						else
							// discourage AI making collateral damage with spells
							ps.value -= 4 * dpsReduce * scoreEvaluator.getNegativeEffectMultiplier();

#if BATTLE_TRACE_LEVEL >= 1
						logAi->trace(
							"Spell affects %s (%d), dps: %2f",
							unit->creatureId().toCreature()->getNameSingularTranslated(),
							unit->getCount(),
							dpsReduce);
#endif
					}
				}

#if BATTLE_TRACE_LEVEL >= 1
				logAi->trace("Total score: %2f", ps.value);
#endif
			}
#if BATTLE_TRACE_LEVEL == 0
		});
#endif

	LOGFL("Evaluation took %d ms", timer.getDiff());

	auto castToPerform = *vstd::maxElementByFun(possibleCasts, [](const PossibleSpellcast & ps) -> float
		{
			return ps.value;
		});

	if(castToPerform.value > cachedAttack.score && !vstd::isAlmostEqual(castToPerform.value, cachedAttack.score))
	{
		LOGFL("Best spell is %s (value %d). Will cast.", castToPerform.spell->getNameTranslated() % castToPerform.value);
		BattleAction spellcast;
		spellcast.actionType = EActionType::HERO_SPELL;
		spellcast.spell = castToPerform.spell->id;
		spellcast.setTarget(castToPerform.dest);
		spellcast.side = side;
		spellcast.stackNumber = -1;
		cb->battleMakeSpellAction(battleID, spellcast);
		activeActionMade = true;

		return true;
	}

	LOGFL("Best spell is %s. But it is actually useless (value %d).", castToPerform.spell->getNameTranslated() % castToPerform.value);

	return false;
}

//Below method works only for offensive spells
void BattleEvaluator::evaluateCreatureSpellcast(const CStack * stack, PossibleSpellcast & ps)
{
	using ValueMap = PossibleSpellcast::ValueMap;

	RNGStub rngStub;
	HypotheticBattle state(env.get(), cb->getBattle(battleID));
	TStacks all = cb->getBattle(battleID)->battleGetAllStacks(false);

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

		if(localUnit->unitOwner() != cb->getBattle(battleID)->getPlayerID())
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

void BattleEvaluator::print(const std::string & text) const
{
	logAi->trace("%s Battle AI[%p]: %s", playerID.toString(), this, text);
}
