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
#include "BattleExchangeVariant.h"
#include "BattleEvaluator.h"
#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"

#include <tbb/parallel_for.h>

AttackerValue::AttackerValue()
	: value(0),
	isRetaliated(false)
{
}

MoveTarget::MoveTarget()
	: positions(), cachedAttack(), score(EvaluationResult::INEFFECTIVE_SCORE)
{
	turnsToReach = 1;
}

float BattleExchangeVariant::trackAttack(
	const AttackPossibility & ap,
	std::shared_ptr<HypotheticBattle> hb,
	DamageCache & damageCache)
{
	if(!ap.attackerState)
	{
		logAi->trace("Skipping fake ap attack");
		return 0;
	}

	auto attacker = hb->getForUpdate(ap.attack.attacker->unitId());

	float attackValue = ap.attackValue();
	auto affectedUnits = ap.affectedUnits;

	dpsScore.ourDamageReduce += ap.attackerDamageReduce + ap.collateralDamageReduce;
	dpsScore.enemyDamageReduce += ap.defenderDamageReduce + ap.shootersBlockedDmg;
	attackerValue[attacker->unitId()].value = attackValue;

	affectedUnits.push_back(ap.attackerState);

	for(auto affectedUnit : affectedUnits)
	{
		auto unitToUpdate = hb->getForUpdate(affectedUnit->unitId());
		auto damageDealt = unitToUpdate->getAvailableHealth() - affectedUnit->getAvailableHealth();

		if(damageDealt > 0)
		{
			unitToUpdate->damage(damageDealt);
		}

		if(unitToUpdate->unitSide() == attacker->unitSide())
		{
			if(unitToUpdate->unitId() == attacker->unitId())
			{
				unitToUpdate->afterAttack(ap.attack.shooting, false);

#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s -> %s, ap retaliation, %s, dps: %lld",
					hb->getForUpdate(ap.attack.defender->unitId())->getDescription(),
					ap.attack.attacker->getDescription(),
					ap.attack.shooting ? "shot" : "mellee",
					damageDealt);
#endif
			}
			else
			{
#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s, ap collateral, dps: %lld",
					unitToUpdate->getDescription(),
					damageDealt);
#endif
			}
		}
		else
		{
			if(unitToUpdate->unitId() == ap.attack.defender->unitId())
			{
				if(unitToUpdate->ableToRetaliate() && !affectedUnit->ableToRetaliate())
				{
					unitToUpdate->afterAttack(ap.attack.shooting, true);
				}

#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s -> %s, ap attack, %s, dps: %lld",
					attacker->getDescription(),
					ap.attack.defender->getDescription(),
					ap.attack.shooting ? "shot" : "mellee",
					damageDealt);
#endif
			}
			else
			{
#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s, ap enemy collateral, dps: %lld",
					unitToUpdate->getDescription(),
					damageDealt);
#endif
			}
		}
	}

#if BATTLE_TRACE_LEVEL >= 1
	logAi->trace(
		"ap score: our: %2f, enemy: %2f, collateral: %2f, blocked: %2f",
		ap.attackerDamageReduce,
		ap.defenderDamageReduce,
		ap.collateralDamageReduce,
		ap.shootersBlockedDmg);
#endif

	return attackValue;
}

float BattleExchangeVariant::trackAttack(
	std::shared_ptr<StackWithBonuses> attacker,
	std::shared_ptr<StackWithBonuses> defender,
	bool shooting,
	bool isOurAttack,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb,
	bool evaluateOnly)
{
	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type()(BonusType::BLOCKS_RETALIATION);
	const bool counterAttacksBlocked = attacker->hasBonus(selectorBlocksRetaliation, cachingStringBlocksRetaliation);

	int64_t attackDamage = damageCache.getDamage(attacker.get(), defender.get(), hb);
	float defenderDamageReduce = AttackPossibility::calculateDamageReduce(attacker.get(), defender.get(), attackDamage, damageCache, hb);
	float attackerDamageReduce = 0;

	if(!evaluateOnly)
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace(
			"%s -> %s, normal attack, %s, dps: %lld, %2f",
			attacker->getDescription(),
			defender->getDescription(),
			shooting ? "shot" : "mellee",
			attackDamage,
			defenderDamageReduce);
#endif

		if(isOurAttack)
		{
			dpsScore.enemyDamageReduce += defenderDamageReduce;
			attackerValue[attacker->unitId()].value += defenderDamageReduce;
		}
		else
			dpsScore.ourDamageReduce += defenderDamageReduce;

		defender->damage(attackDamage);
		attacker->afterAttack(shooting, false);
	}

	if(!evaluateOnly && defender->alive() && defender->ableToRetaliate() && !counterAttacksBlocked && !shooting)
	{
		auto retaliationDamage = damageCache.getDamage(defender.get(), attacker.get(), hb);
		attackerDamageReduce = AttackPossibility::calculateDamageReduce(defender.get(), attacker.get(), retaliationDamage, damageCache, hb);

#if BATTLE_TRACE_LEVEL>=1
		logAi->trace(
			"%s -> %s, retaliation, dps: %lld, %2f",
			defender->getDescription(),
			attacker->getDescription(),
			retaliationDamage,
			attackerDamageReduce);
#endif

		if(isOurAttack)
		{
			dpsScore.ourDamageReduce += attackerDamageReduce;
			attackerValue[attacker->unitId()].isRetaliated = true;
		}
		else
		{
			dpsScore.enemyDamageReduce += attackerDamageReduce;
			attackerValue[defender->unitId()].value += attackerDamageReduce;
		}

		attacker->damage(retaliationDamage);
		defender->afterAttack(false, true);
	}

	auto score = defenderDamageReduce - attackerDamageReduce;

#if BATTLE_TRACE_LEVEL>=1
	if(!score)
	{
		logAi->trace("Attack has zero score def:%2f att:%2f", defenderDamageReduce, attackerDamageReduce);
	}
#endif

	return score;
}

float BattleExchangeEvaluator::scoreValue(const BattleScore & score) const
{
	return score.enemyDamageReduce * getPositiveEffectMultiplier() - score.ourDamageReduce * getNegativeEffectMultiplier();
}

EvaluationResult BattleExchangeEvaluator::findBestTarget(
	const battle::Unit * activeStack,
	PotentialTargets & targets,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb,
	bool siegeDefense)
{
	EvaluationResult result(targets.bestAction());

	if(!activeStack->waited() && !activeStack->acquireState()->hadMorale)
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace("Evaluating waited attack for %s", activeStack->getDescription());
#endif

		auto hbWaited = std::make_shared<HypotheticBattle>(env.get(), hb);

		hbWaited->makeWait(activeStack);

		updateReachabilityMap(hbWaited);

		for(auto & ap : targets.possibleAttacks)
		{
			if (siegeDefense && !hb->battleIsInsideWalls(ap.from))
				continue;

			float score = evaluateExchange(ap, 0, targets, damageCache, hbWaited);

			if(score > result.score)
			{
				result.score = score;
				result.bestAttack = ap;
				result.wait = true;

#if BATTLE_TRACE_LEVEL >= 1
				logAi->trace("New high score %2f", result.score);
#endif
			}
		}
	}

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Evaluating normal attack for %s", activeStack->getDescription());
#endif

	updateReachabilityMap(hb);

	if(result.bestAttack.attack.shooting
		&& !result.bestAttack.defenderDead
		&& !activeStack->waited()
		&& hb->battleHasShootingPenalty(activeStack, result.bestAttack.dest))
	{
		if(!canBeHitThisTurn(result.bestAttack))
			return result; // lets wait
	}

	for(auto & ap : targets.possibleAttacks)
	{
		if (siegeDefense && !hb->battleIsInsideWalls(ap.from))
			continue;

		float score = evaluateExchange(ap, 0, targets, damageCache, hb);
		bool sameScoreButWaited = vstd::isAlmostEqual(score, result.score) && result.wait;

		if(score > result.score || sameScoreButWaited)
		{
			result.score = score;
			result.bestAttack = ap;
			result.wait = false;

#if BATTLE_TRACE_LEVEL >= 1
			logAi->trace("New high score %2f", result.score);
#endif
		}
	}

	return result;
}

ReachabilityInfo getReachabilityWithEnemyBypass(
	const battle::Unit * activeStack,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> state)
{
	ReachabilityInfo::Parameters params(activeStack, activeStack->getPosition());

	if(!params.flying)
	{
		for(const auto * unit : state->battleAliveUnits())
		{
			if(unit->unitSide() == activeStack->unitSide())
				continue;

			auto dmg = damageCache.getOriginalDamage(activeStack, unit, state);
			auto turnsToKill = unit->getAvailableHealth() / std::max(dmg, (int64_t)1);

			vstd::amin(turnsToKill, 100);

			for(auto & hex : unit->getHexes())
				if(hex.isAvailable()) //towers can have <0 pos; we don't also want to overwrite side columns
					params.destructibleEnemyTurns[hex.toInt()] = turnsToKill * unit->getMovementRange();
		}

		params.bypassEnemyStacks = true;
	}

	return state->getReachability(params);
}

MoveTarget BattleExchangeEvaluator::findMoveTowardsUnreachable(
	const battle::Unit * activeStack,
	PotentialTargets & targets,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb)
{
	MoveTarget result;
	BattleExchangeVariant ev;

	logAi->trace("Find move towards unreachable. Enemies count %d", targets.unreachableEnemies.size());

	if(targets.unreachableEnemies.empty())
		return result;

	auto speed = activeStack->getMovementRange();

	if(speed == 0)
		return result;

	updateReachabilityMap(hb);

	auto dists = getReachabilityWithEnemyBypass(activeStack, damageCache, hb);
	auto flying = activeStack->hasBonusOfType(BonusType::FLYING);

	for(const battle::Unit * enemy : targets.unreachableEnemies)
	{
		logAi->trace(
			"Checking movement towards %d of %s",
			enemy->getCount(),
			LIBRARY->creatures()->getById(enemy->creatureId())->getJsonKey());

		auto distance = dists.distToNearestNeighbour(activeStack, enemy);

		if(distance >= GameConstants::BFIELD_SIZE)
			continue;

		if(distance <= speed)
			continue;

		float penaltyMultiplier = 1.0f; // Default multiplier, no penalty
		float closestAllyDistance = std::numeric_limits<float>::max();

		for (const battle::Unit* ally : hb->battleAliveUnits()) 
		{
			if (ally == activeStack) 
				continue;
			if (ally->unitSide() != activeStack->unitSide()) 
				continue;

			float allyDistance = dists.distToNearestNeighbour(ally, enemy);
			if (allyDistance < closestAllyDistance)
			{
				closestAllyDistance = allyDistance;
			}
		}

		// If an ally is closer to the enemy, compute the penaltyMultiplier
		if (closestAllyDistance < distance) 
		{
			penaltyMultiplier = closestAllyDistance / distance; // Ratio of distances
		}

		auto turnsToReach = (distance - 1) / speed + 1;
		const BattleHexArray & hexes = enemy->getAttackableHexes(activeStack);
		auto enemySpeed = enemy->getMovementRange();
		auto speedRatio = speed / static_cast<float>(enemySpeed);
		auto multiplier = (speedRatio > 1 ? 1 : speedRatio) * penaltyMultiplier;

		for(auto & hex : hexes)
		{
			if (!dists.isReachable(hex))
				continue;
			// FIXME: provide distance info for Jousting bonus
			auto bai = BattleAttackInfo(activeStack, enemy, 0, cb->battleCanShoot(activeStack));
			auto attack = AttackPossibility::evaluate(bai, hex, damageCache, hb);

			attack.shootersBlockedDmg = 0; // we do not want to count on it, it is not for sure

			auto score = calculateExchange(attack, turnsToReach, targets, damageCache, hb);

			score.enemyDamageReduce *= multiplier;

#if BATTLE_TRACE_LEVEL >= 1
			logAi->trace("Multiplier: %f, turns: %d, current score %f, new score %f", multiplier, turnsToReach, result.score, scoreValue(score));
#endif

			if(result.score < scoreValue(score)
				|| (result.turnsToReach > turnsToReach && vstd::isAlmostEqual(result.score, scoreValue(score))))
			{
				result.score = scoreValue(score);
				result.positions.clear();

#if BATTLE_TRACE_LEVEL >= 1
				logAi->trace("New high score");
#endif

					BattleHex enemyHex = hex;
					while(!flying && dists.distances[enemyHex.toInt()] > speed && dists.predecessors.at(enemyHex.toInt()).isValid())
					{
						enemyHex = dists.predecessors.at(enemyHex.toInt());

						if(dists.accessibility[enemyHex.toInt()] == EAccessibility::ALIVE_STACK)
						{
							auto defenderToBypass = hb->battleGetUnitByPos(enemyHex);
							assert(defenderToBypass != nullptr);
							auto attackHex = dists.predecessors[enemyHex.toInt()];
							
							if(defenderToBypass &&
							   defenderToBypass != enemy &&
							   vstd::contains(defenderToBypass->getAttackableHexes(activeStack), attackHex))
							{
#if BATTLE_TRACE_LEVEL >= 1
								logAi->trace("Found target to bypass at %d", enemyHex.toInt());
#endif
								
								auto baiBypass = BattleAttackInfo(activeStack, defenderToBypass, 0, cb->battleCanShoot(activeStack));
								auto attackBypass = AttackPossibility::evaluate(baiBypass, attackHex, damageCache, hb);

								auto adjacentStacks = getAdjacentUnits(enemy);

								adjacentStacks.push_back(defenderToBypass);
								vstd::removeDuplicates(adjacentStacks);

								auto bypassScore = calculateExchange(
									attackBypass,
									dists.distances[attackHex.toInt()],
									targets,
									damageCache,
									hb,
									adjacentStacks);

								if(scoreValue(bypassScore) > result.score)
								{
									result.score = scoreValue(bypassScore);

#if BATTLE_TRACE_LEVEL >= 1
									logAi->trace("New high score after bypass %f", scoreValue(bypassScore));
#endif
								}
							}
						}
					}

				result.positions.insert(enemyHex);
				result.cachedAttack = attack;
				result.turnsToReach = turnsToReach;
			}
		}
	}

	return result;
}

battle::Units BattleExchangeEvaluator::getAdjacentUnits(const battle::Unit * blockerUnit) const
{
	std::queue<const battle::Unit *> queue;
	battle::Units checkedStacks;

	queue.push(blockerUnit);

	while(!queue.empty())
	{
		auto stack = queue.front();

		queue.pop();
		checkedStacks.push_back(stack);

		auto const & hexes = stack->getSurroundingHexes();
		for(const auto & hex : hexes)
		{
			auto neighbour = cb->battleGetUnitByPos(hex);

			if(neighbour && neighbour->unitSide() == stack->unitSide() && !vstd::contains(checkedStacks, neighbour))
			{
				queue.push(neighbour);
				checkedStacks.push_back(neighbour);
			}
		}
	}

	return checkedStacks;
}

ReachabilityData BattleExchangeEvaluator::getExchangeUnits(
	const AttackPossibility & ap,
	uint8_t turn,
	PotentialTargets & targets,
	std::shared_ptr<HypotheticBattle> hb,
	const battle::Units & additionalUnits) const
{
	ReachabilityData result;

	auto hexes = ap.attack.defender->getSurroundingHexes();

	if(!ap.attack.shooting) 
		hexes.insert(ap.from);

	battle::Units allReachableUnits = additionalUnits;
	
	for(const auto & hex : hexes)
	{
		vstd::concatenate(allReachableUnits, getOneTurnReachableUnits(turn, hex));
	}

	if(!ap.attack.attacker->isTurret())
	{
		for(const auto & hex : ap.attack.attacker->getHexes())
		{
			auto unitsReachingAttacker = getOneTurnReachableUnits(turn, hex);
			for(auto unit : unitsReachingAttacker)
			{
				if(unit->unitSide() != ap.attack.attacker->unitSide())
				{
					allReachableUnits.push_back(unit);
					result.enemyUnitsReachingAttacker.insert(unit->unitId());
				}
			}
		}
	}

	vstd::removeDuplicates(allReachableUnits);

	auto copy = allReachableUnits;
	for(auto unit : copy)
	{
		for(auto adjacentUnit : getAdjacentUnits(unit))
		{
			auto unitWithBonuses = hb->battleGetUnitByID(adjacentUnit->unitId());

			if(vstd::contains(targets.unreachableEnemies, adjacentUnit)
				&& !vstd::contains(allReachableUnits, unitWithBonuses))
			{
				allReachableUnits.push_back(unitWithBonuses);
			}
		}
	}

	vstd::removeDuplicates(allReachableUnits);

	if(!vstd::contains(allReachableUnits, ap.attack.attacker))
	{
		allReachableUnits.push_back(ap.attack.attacker);
	}

	if(allReachableUnits.size() < 2)
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace("Reachability map contains only %d stacks", allReachableUnits.size());
#endif

		return result;
	}

	for(auto unit : allReachableUnits)
	{
		auto accessible = !unit->canShoot() || vstd::contains(additionalUnits, unit);

		if(!accessible)
		{
			for(const auto & hex : unit->getSurroundingHexes())
			{
				if(ap.attack.defender->coversPos(hex))
				{
					accessible = true;
				}
			}
		}

		if(accessible)
			result.melleeAccessible.push_back(unit);
		else
			result.shooters.push_back(unit);
	}

	for(int turn = 0; turn < turnOrder.size(); turn++)
	{
		for(auto unit : turnOrder[turn])
		{
			if(vstd::contains(allReachableUnits, unit))
				result.units[turn].push_back(unit);
		}

		vstd::erase_if(result.units[turn], [&](const battle::Unit * u) -> bool
			{
				return !hb->battleGetUnitByID(u->unitId())->alive();
			});
	}

	return result;
}

float BattleExchangeEvaluator::evaluateExchange(
	const AttackPossibility & ap,
	uint8_t turn,
	PotentialTargets & targets,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb) const
{
	BattleScore score = calculateExchange(ap, turn, targets, damageCache, hb);

#if BATTLE_TRACE_LEVEL >= 1
	logAi->trace(
		"calculateExchange score +%2f -%2fx%2f = %2f",
		score.enemyDamageReduce,
		score.ourDamageReduce,
		getNegativeEffectMultiplier(),
		scoreValue(score));
#endif

	return scoreValue(score);
}

BattleScore BattleExchangeEvaluator::calculateExchange(
	const AttackPossibility & ap,
	uint8_t turn,
	PotentialTargets & targets,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb,
	const battle::Units & additionalUnits) const
{
#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Battle exchange at %d", ap.attack.shooting ? ap.dest.toInt() : ap.from.toInt());
#endif

	if(cb->battleGetMySide() == BattleSide::LEFT_SIDE
		&& cb->battleGetGateState() == EGateState::BLOCKED
		&& ap.attack.defender->coversPos(BattleHex::GATE_BRIDGE))
	{
		return BattleScore(EvaluationResult::INEFFECTIVE_SCORE, 0);
	}

	battle::Units ourStacks;
	battle::Units enemyStacks;

	if(hb->battleGetUnitByID(ap.attack.defender->unitId())->alive())
		enemyStacks.push_back(ap.attack.defender);

	ReachabilityData exchangeUnits = getExchangeUnits(ap, turn, targets, hb, additionalUnits);

	if(exchangeUnits.units.empty())
	{
		return BattleScore();
	}

	auto exchangeBattle = std::make_shared<HypotheticBattle>(env.get(), hb);
	BattleExchangeVariant v;

	for(int exchangeTurn = 0; exchangeTurn < exchangeUnits.units.size(); exchangeTurn++)
	{
		for(auto unit : exchangeUnits.units.at(exchangeTurn))
		{
			if(unit->isTurret())
				continue;

			bool isOur = exchangeBattle->battleMatchOwner(ap.attack.attacker, unit, true);
			auto & attackerQueue = isOur ? ourStacks : enemyStacks;
			auto u = exchangeBattle->getForUpdate(unit->unitId());

			if(u->alive() && !vstd::contains(attackerQueue, unit))
			{
				attackerQueue.push_back(unit);

#if BATTLE_TRACE_LEVEL
				logAi->trace("Exchanging: %s", u->getDescription());
#endif
			}
		}
	}

	auto melleeAttackers = ourStacks;

	vstd::removeDuplicates(melleeAttackers);
	vstd::erase_if(melleeAttackers, [&](const battle::Unit * u) -> bool
		{
			return cb->battleCanShoot(u);
		});

	bool canUseAp = true;

	std::set<uint32_t> blockedShooters;

	int totalTurnsCount = simulationTurnsCount >= turn + turnOrder.size()
		? simulationTurnsCount
		: turn + turnOrder.size();

	for(int exchangeTurn = 0; exchangeTurn < simulationTurnsCount; exchangeTurn++)
	{
		bool isMovingTurm = exchangeTurn < turn;
		int queueTurn = exchangeTurn >= exchangeUnits.units.size()
			? exchangeUnits.units.size() - 1
			: exchangeTurn;

		for(auto activeUnit : exchangeUnits.units.at(queueTurn))
		{
			bool isOur = exchangeBattle->battleMatchOwner(ap.attack.attacker, activeUnit, true);
			battle::Units & attackerQueue = isOur ? ourStacks : enemyStacks;
			battle::Units & oppositeQueue = isOur ? enemyStacks : ourStacks;

			auto attacker = exchangeBattle->getForUpdate(activeUnit->unitId());
			auto shooting = exchangeBattle->battleCanShoot(attacker.get())
				&& !vstd::contains(blockedShooters, attacker->unitId());

			if(!attacker->alive())
			{
#if BATTLE_TRACE_LEVEL>=1
				logAi->trace("Attacker is dead");
#endif

				continue;
			}

			if(isMovingTurm && !shooting
				&& !vstd::contains(exchangeUnits.enemyUnitsReachingAttacker, attacker->unitId()))
			{
#if BATTLE_TRACE_LEVEL>=1
				logAi->trace("Attacker is moving");
#endif

				continue;
			}

			auto targetUnit = ap.attack.defender;

			if(!isOur || !exchangeBattle->battleGetUnitByID(targetUnit->unitId())->alive())
			{
#if BATTLE_TRACE_LEVEL>=2
				logAi->trace("Best target selector for %s", attacker->getDescription());
#endif
				auto estimateAttack = [&](const battle::Unit * u) -> float
				{
					auto stackWithBonuses = exchangeBattle->getForUpdate(u->unitId());
					auto score = v.trackAttack(
						attacker,
						stackWithBonuses,
						exchangeBattle->battleCanShoot(stackWithBonuses.get()),
						isOur,
						damageCache,
						hb,
						true);

#if BATTLE_TRACE_LEVEL>=2
					logAi->trace("Best target selector %s->%s score = %2f", attacker->getDescription(), stackWithBonuses->getDescription(), score);
#endif

					return score;
				};

				auto unitsInOppositeQueueExceptInaccessible = oppositeQueue;

				vstd::erase_if(unitsInOppositeQueueExceptInaccessible, [&](const battle::Unit * u)->bool
					{
						return vstd::contains(exchangeUnits.shooters, u);
					});

				if(!isOur
					&& exchangeTurn == 0
					&& exchangeUnits.units.at(exchangeTurn).at(0)->unitId() != ap.attack.attacker->unitId()
					&& !vstd::contains(exchangeUnits.enemyUnitsReachingAttacker, attacker->unitId()))
				{
					vstd::erase_if(unitsInOppositeQueueExceptInaccessible, [&](const battle::Unit * u) -> bool
						{
							return u->unitId() == ap.attack.attacker->unitId();
						});
				}

				if(!unitsInOppositeQueueExceptInaccessible.empty())
				{
					targetUnit = *vstd::maxElementByFun(unitsInOppositeQueueExceptInaccessible, estimateAttack);
				}
				else
				{
					auto reachable = exchangeBattle->battleGetUnitsIf([this, &exchangeBattle, &attacker](const battle::Unit * u) -> bool
						{
							if(u->unitSide() == attacker->unitSide())
								return false;

							if(!exchangeBattle->getForUpdate(u->unitId())->alive())
								return false;

							if(!u->getPosition().isValid())
								return false; // e.g. tower shooters

							const auto & reachableUnits = getOneTurnReachableUnits(0, u->getPosition());

							return vstd::contains_if(reachableUnits, [&attacker](const battle::Unit * other) -> bool
								{
									return attacker->unitId() == other->unitId();
								});
						});

					if(!reachable.empty())
					{
						targetUnit = *vstd::maxElementByFun(reachable, estimateAttack);
					}
					else
					{
#if BATTLE_TRACE_LEVEL>=1
						logAi->trace("Battle queue is empty and no reachable enemy.");
#endif

						continue;
					}
				}
			}

			auto defender = exchangeBattle->getForUpdate(targetUnit->unitId());
			const int totalAttacks = attacker->getTotalAttacks(shooting);

			if(canUseAp && activeUnit->unitId() == ap.attack.attacker->unitId()
				&& targetUnit->unitId() == ap.attack.defender->unitId())
			{
				v.trackAttack(ap, exchangeBattle, damageCache);
			}
			else
			{
				for(int i = 0; i < totalAttacks; i++)
				{
					v.trackAttack(attacker, defender, shooting, isOur, damageCache, exchangeBattle);

					if(!attacker->alive() || !defender->alive())
						break;
				}
			}

			if(!shooting)
				blockedShooters.insert(defender->unitId());

			canUseAp = false;

			vstd::erase_if(attackerQueue, [&](const battle::Unit * u) -> bool
				{
					return !exchangeBattle->battleGetUnitByID(u->unitId())->alive();
				});

			vstd::erase_if(oppositeQueue, [&](const battle::Unit * u) -> bool
				{
					return !exchangeBattle->battleGetUnitByID(u->unitId())->alive();
				});
		}

		exchangeBattle->nextRound();
	}

	auto score = v.getScore();

	if(simulationTurnsCount < totalTurnsCount)
	{
		float scalingRatio = simulationTurnsCount / static_cast<float>(totalTurnsCount);

		score.enemyDamageReduce *= scalingRatio;
		score.ourDamageReduce *= scalingRatio;
	}

	if(turn > 0)
	{
		auto turnMultiplier = 1 - std::min(0.2, 0.05 * turn);

		score.enemyDamageReduce *= turnMultiplier;
	}

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Exchange score: enemy: %2f, our -%2f", score.enemyDamageReduce, score.ourDamageReduce);
#endif

	return score;
}

bool BattleExchangeEvaluator::canBeHitThisTurn(const AttackPossibility & ap)
{
	for(auto pos : ap.attack.attacker->getSurroundingHexes())
	{
		for(auto u : getOneTurnReachableUnits(0, pos))
		{
			if(u->unitSide() != ap.attack.attacker->unitSide())
			{
				return true;
			}
		}
	}

	return false;
}

void ReachabilityMapCache::update(const std::vector<battle::Units> & turnOrder, std::shared_ptr<HypotheticBattle> hb)
{
	for(auto turn : turnOrder)
	{
		for(auto u : turn)
		{
			if(!vstd::contains(unitReachabilityMap, u->unitId()))
			{
				unitReachabilityMap[u->unitId()] = hb->getReachability(u);
			}
		}
	}

	hexReachabilityPerTurn.clear();
}

void BattleExchangeEvaluator::updateReachabilityMap(std::shared_ptr<HypotheticBattle> hb)
{
	const int TURN_DEPTH = 2;

	turnOrder.clear();

	hb->battleGetTurnOrder(turnOrder, std::numeric_limits<int>::max(), TURN_DEPTH);
	reachabilityMap.update(turnOrder, hb);
}

const battle::Units & ReachabilityMapCache::getOneTurnReachableUnits(std::shared_ptr<CBattleInfoCallback> cb, std::shared_ptr<Environment> env, const std::vector<battle::Units> & turnOrder, uint8_t turn, const BattleHex & hex)
{
	auto & turnData = hexReachabilityPerTurn[turn];

	if (!turnData.isValid[hex.toInt()])
	{
		turnData.hexes[hex.toInt()] = computeOneTurnReachableUnits(cb, env, turnOrder, turn, hex);
		turnData.isValid.set(hex.toInt());
	}

	return turnData.hexes[hex.toInt()];
}

battle::Units ReachabilityMapCache::computeOneTurnReachableUnits(std::shared_ptr<CBattleInfoCallback> cb, std::shared_ptr<Environment> env, const std::vector<battle::Units> & turnOrder, uint8_t turn, const BattleHex & hex)
{
	battle::Units result;

	for(int i = 0; i < turnOrder.size(); i++, turn++)
	{
		auto & turnQueue = turnOrder[i];
		HypotheticBattle turnBattle(env.get(), cb);

		for(const battle::Unit * unit : turnQueue)
		{
			if(unit->isTurret())
				continue;

			if(turnBattle.battleCanShoot(unit))
			{
				result.push_back(unit);

				continue;
			}

			auto unitSpeed = unit->getMovementRange(turn);
			auto radius = unitSpeed * (turn + 1);

			auto reachabilityIter = unitReachabilityMap.find(unit->unitId());
			assert(reachabilityIter != unitReachabilityMap.end()); // missing updateReachabilityMap call?

			ReachabilityInfo unitReachability = reachabilityIter != unitReachabilityMap.end() ? reachabilityIter->second : turnBattle.getReachability(unit);

			bool reachable = unitReachability.distances.at(hex.toInt()) <= radius;

			if(!reachable && unitReachability.accessibility[hex.toInt()] == EAccessibility::ALIVE_STACK)
			{
				const battle::Unit * hexStack = cb->battleGetUnitByPos(hex);

				if(hexStack && cb->battleMatchOwner(unit, hexStack, false))
				{
					for(const BattleHex & neighbour : hex.getNeighbouringTiles())
					{
						reachable = unitReachability.distances.at(neighbour.toInt()) <= radius;

						if(reachable) break;
					}
				}
			}

			if(reachable)
			{
				result.push_back(unit);
			}
		}
	}

	return result;
}

const battle::Units & BattleExchangeEvaluator::getOneTurnReachableUnits(uint8_t turn, const BattleHex & hex) const
{
	return reachabilityMap.getOneTurnReachableUnits(cb, env, turnOrder, turn, hex);
}

// avoid blocking path for stronger stack by weaker stack
bool BattleExchangeEvaluator::checkPositionBlocksOurStacks(const HypotheticBattle & hb, const battle::Unit * activeUnit, const BattleHex & position)
{
	const int BLOCKING_THRESHOLD = 70;
	const int BLOCKING_OWN_ATTACK_PENALTY = 100;
	const int BLOCKING_OWN_MOVE_PENALTY = 1;

	float blockingScore = 0;

	auto activeUnitDamage = activeUnit->getMinDamage(hb.battleCanShoot(activeUnit)) * activeUnit->getCount();

	for(int turn = 0; turn < turnOrder.size(); turn++)
	{
		auto & turnQueue = turnOrder[turn];
		HypotheticBattle turnBattle(env.get(), cb);

		auto unitToUpdate = turnBattle.getForUpdate(activeUnit->unitId());
		unitToUpdate->setPosition(position);

		for(const battle::Unit * unit : turnQueue)
		{
			if(unit->unitId() == unitToUpdate->unitId() || cb->battleMatchOwner(unit, activeUnit, false))
				continue;

			auto blockedUnitDamage = unit->getMinDamage(hb.battleCanShoot(unit)) * unit->getCount();
			float ratio = blockedUnitDamage / (float)(blockedUnitDamage + activeUnitDamage + 0.01);

			auto unitReachability = turnBattle.getReachability(unit);
			auto unitSpeed = unit->getMovementRange(turn); // Cached value, to avoid performance hit

			for(BattleHex hex = BattleHex::TOP_LEFT; hex.isValid(); ++hex)
			{
				bool enemyUnit = false;
				bool reachable = unitReachability.distances.at(hex.toInt()) <= unitSpeed;

				if(!reachable && unitReachability.accessibility[hex.toInt()] == EAccessibility::ALIVE_STACK)
				{
					const battle::Unit * hexStack = turnBattle.battleGetUnitByPos(hex);

					if(hexStack && cb->battleMatchOwner(unit, hexStack, false))
					{
						enemyUnit = true;
						for(const BattleHex & neighbour : hex.getNeighbouringTiles())
						{
							reachable = unitReachability.distances.at(neighbour.toInt()) <= unitSpeed;

							if(reachable) break;
						}
					}
				}

				if(!reachable)
				{
					auto reachableUnits = getOneTurnReachableUnits(0, hex);
					if (std::count(reachableUnits.begin(), reachableUnits.end(), unit) > 1)
						blockingScore += ratio * (enemyUnit ? BLOCKING_OWN_ATTACK_PENALTY : BLOCKING_OWN_MOVE_PENALTY);
				}
			}
		}
	}

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Position %d, blocking score %f", position.toInt(), blockingScore);
#endif

	return blockingScore > BLOCKING_THRESHOLD;
}
