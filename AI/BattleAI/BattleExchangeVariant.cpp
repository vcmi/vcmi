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
#include "../../lib/CStack.h"

AttackerValue::AttackerValue()
	: value(0),
	isRetalitated(false)
{
}

MoveTarget::MoveTarget()
	: positions(), cachedAttack(), score(EvaluationResult::INEFFECTIVE_SCORE), scorePerTurn(EvaluationResult::INEFFECTIVE_SCORE)
{
	turnsToRich = 1;
}

float BattleExchangeVariant::trackAttack(
	const AttackPossibility & ap,
	std::shared_ptr<HypotheticBattle> hb,
	DamageCache & damageCache)
{
	auto attacker = hb->getForUpdate(ap.attack.attacker->unitId());

	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type()(BonusType::BLOCKS_RETALIATION);
	const bool counterAttacksBlocked = attacker->hasBonus(selectorBlocksRetaliation, cachingStringBlocksRetaliation);

	float attackValue = 0;
	auto affectedUnits = ap.affectedUnits;

	affectedUnits.push_back(ap.attackerState);

	for(auto affectedUnit : affectedUnits)
	{
		auto unitToUpdate = hb->getForUpdate(affectedUnit->unitId());

		if(unitToUpdate->unitSide() == attacker->unitSide())
		{
			if(unitToUpdate->unitId() == attacker->unitId())
			{
				auto defender = hb->getForUpdate(ap.attack.defender->unitId());

				if(!defender->alive() || counterAttacksBlocked || ap.attack.shooting || !defender->ableToRetaliate())
					continue;

				auto retaliationDamage = damageCache.getDamage(defender.get(), unitToUpdate.get(), hb);
				auto attackerDamageReduce = AttackPossibility::calculateDamageReduce(defender.get(), unitToUpdate.get(), retaliationDamage, damageCache, hb);

				attackValue -= attackerDamageReduce;
				dpsScore.ourDamageReduce += attackerDamageReduce;
				attackerValue[unitToUpdate->unitId()].isRetalitated = true;

				unitToUpdate->damage(retaliationDamage);
				defender->afterAttack(false, true);

#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s -> %s, ap retalitation, %s, dps: %2f, score: %2f",
					defender->getDescription(),
					unitToUpdate->getDescription(),
					ap.attack.shooting ? "shot" : "mellee",
					retaliationDamage,
					attackerDamageReduce);
#endif
			}
			else
			{
				auto collateralDamage = damageCache.getDamage(attacker.get(), unitToUpdate.get(), hb);
				auto collateralDamageReduce = AttackPossibility::calculateDamageReduce(attacker.get(), unitToUpdate.get(), collateralDamage, damageCache, hb);

				attackValue -= collateralDamageReduce;
				dpsScore.ourDamageReduce += collateralDamageReduce;

				unitToUpdate->damage(collateralDamage);

#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s -> %s, ap collateral, %s, dps: %2f, score: %2f",
					attacker->getDescription(),
					unitToUpdate->getDescription(),
					ap.attack.shooting ? "shot" : "mellee",
					collateralDamage,
					collateralDamageReduce);
#endif
			}
		}
		else
		{
			int64_t attackDamage = damageCache.getDamage(attacker.get(), unitToUpdate.get(), hb);
			float defenderDamageReduce = AttackPossibility::calculateDamageReduce(attacker.get(), unitToUpdate.get(), attackDamage, damageCache, hb);

			attackValue += defenderDamageReduce;
			dpsScore.enemyDamageReduce += defenderDamageReduce;
			attackerValue[attacker->unitId()].value += defenderDamageReduce;

			unitToUpdate->damage(attackDamage);

#if BATTLE_TRACE_LEVEL>=1
			logAi->trace(
				"%s -> %s, ap attack, %s, dps: %2f, score: %2f",
				attacker->getDescription(),
				unitToUpdate->getDescription(),
				ap.attack.shooting ? "shot" : "mellee",
				attackDamage,
				defenderDamageReduce);
#endif
		}
	}

#if BATTLE_TRACE_LEVEL >= 1
	logAi->trace("ap shooters blocking: %lld", ap.shootersBlockedDmg);
#endif

	attackValue += ap.shootersBlockedDmg;
	dpsScore.enemyDamageReduce += ap.shootersBlockedDmg;
	attacker->afterAttack(ap.attack.shooting, false);

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
			attackerValue[attacker->unitId()].isRetalitated = true;
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
	std::shared_ptr<HypotheticBattle> hb)
{
	EvaluationResult result(targets.bestAction());

	if(!activeStack->waited() && !activeStack->acquireState()->hadMorale)
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace("Evaluating waited attack for %s", activeStack->getDescription());
#endif

		auto hbWaited = std::make_shared<HypotheticBattle>(env.get(), hb);

		hbWaited->getForUpdate(activeStack->unitId())->waiting = true;
		hbWaited->getForUpdate(activeStack->unitId())->waitedThisTurn = true;

		updateReachabilityMap(hbWaited);

		for(auto & ap : targets.possibleAttacks)
		{
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
		&& !activeStack->waited()
		&& hb->battleHasShootingPenalty(activeStack, result.bestAttack.dest))
	{
		if(!canBeHitThisTurn(result.bestAttack))
			return result; // lets wait
	}

	for(auto & ap : targets.possibleAttacks)
	{
		float score = evaluateExchange(ap, 0, targets, damageCache, hb);

		if(score > result.score || (vstd::isAlmostEqual(score, result.score) && result.wait))
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

MoveTarget BattleExchangeEvaluator::findMoveTowardsUnreachable(
	const battle::Unit * activeStack,
	PotentialTargets & targets,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb)
{
	MoveTarget result;
	BattleExchangeVariant ev;

	if(targets.unreachableEnemies.empty())
		return result;

	auto speed = activeStack->getMovementRange();

	if(speed == 0)
		return result;

	updateReachabilityMap(hb);

	auto dists = cb->getReachability(activeStack);

	for(const battle::Unit * enemy : targets.unreachableEnemies)
	{
		std::vector<const battle::Unit *> adjacentStacks = getAdjacentUnits(enemy);
		auto closestStack = *vstd::minElementByFun(adjacentStacks, [&](const battle::Unit * u) -> int64_t
			{
				return dists.distToNearestNeighbour(activeStack, u) * 100000 - activeStack->getTotalHealth();
			});

		auto distance = dists.distToNearestNeighbour(activeStack, closestStack);

		if(distance >= GameConstants::BFIELD_SIZE)
			continue;

		if(distance <= speed)
			continue;

		auto turnsToRich = (distance - 1) / speed + 1;
		auto hexes = closestStack->getSurroundingHexes();
		auto enemySpeed = closestStack->getMovementRange();
		auto speedRatio = speed / static_cast<float>(enemySpeed);
		auto multiplier = speedRatio > 1 ? 1 : speedRatio;

		if(enemy->canShoot())
			multiplier *= 1.5f;

		for(auto hex : hexes)
		{
			// FIXME: provide distance info for Jousting bonus
			auto bai = BattleAttackInfo(activeStack, closestStack, 0, cb->battleCanShoot(activeStack));
			auto attack = AttackPossibility::evaluate(bai, hex, damageCache, hb);

			attack.shootersBlockedDmg = 0; // we do not want to count on it, it is not for sure

			auto score = calculateExchange(attack, turnsToRich, targets, damageCache, hb);
			auto scorePerTurn = BattleScore(score.enemyDamageReduce * std::sqrt(multiplier / turnsToRich), score.ourDamageReduce);

			if(result.scorePerTurn < scoreValue(scorePerTurn))
			{
				result.scorePerTurn = scoreValue(scorePerTurn);
				result.score = scoreValue(score);
				result.positions = closestStack->getAttackableHexes(activeStack);
				result.cachedAttack = attack;
				result.turnsToRich = turnsToRich;
			}
		}
	}

	return result;
}

std::vector<const battle::Unit *> BattleExchangeEvaluator::getAdjacentUnits(const battle::Unit * blockerUnit) const
{
	std::queue<const battle::Unit *> queue;
	std::vector<const battle::Unit *> checkedStacks;

	queue.push(blockerUnit);

	while(!queue.empty())
	{
		auto stack = queue.front();

		queue.pop();
		checkedStacks.push_back(stack);

		auto hexes = stack->getSurroundingHexes();
		for(auto hex : hexes)
		{
			auto neighbor = cb->battleGetUnitByPos(hex);

			if(neighbor && neighbor->unitSide() == stack->unitSide() && !vstd::contains(checkedStacks, neighbor))
			{
				queue.push(neighbor);
				checkedStacks.push_back(neighbor);
			}
		}
	}

	return checkedStacks;
}

ReachabilityData BattleExchangeEvaluator::getExchangeUnits(
	const AttackPossibility & ap,
	uint8_t turn,
	PotentialTargets & targets,
	std::shared_ptr<HypotheticBattle> hb)
{
	ReachabilityData result;

	auto hexes = ap.attack.defender->getSurroundingHexes();

	if(!ap.attack.shooting) hexes.push_back(ap.from);

	std::vector<const battle::Unit *> allReachableUnits;

	for(auto hex : hexes)
	{
		vstd::concatenate(allReachableUnits, turn == 0 ? reachabilityMap[hex] : getOneTurnReachableUnits(turn, hex));
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
		auto accessible = !unit->canShoot();

		if(!accessible)
		{
			for(auto hex : unit->getSurroundingHexes())
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
				result.units.push_back(unit);
		}
	}

	vstd::erase_if(result.units, [&](const battle::Unit * u) -> bool
		{
			return !hb->battleGetUnitByID(u->unitId())->alive();
		});

	return result;
}

float BattleExchangeEvaluator::evaluateExchange(
	const AttackPossibility & ap,
	uint8_t turn,
	PotentialTargets & targets,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> hb)
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
	std::shared_ptr<HypotheticBattle> hb)
{
#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Battle exchange at %d", ap.attack.shooting ? ap.dest.hex : ap.from.hex);
#endif

	if(cb->battleGetMySide() == BattlePerspective::LEFT_SIDE
		&& cb->battleGetGateState() == EGateState::BLOCKED
		&& ap.attack.defender->coversPos(BattleHex::GATE_BRIDGE))
	{
		return BattleScore(EvaluationResult::INEFFECTIVE_SCORE, 0);
	}

	std::vector<const battle::Unit *> ourStacks;
	std::vector<const battle::Unit *> enemyStacks;

	if(hb->battleGetUnitByID(ap.attack.defender->unitId())->alive())
		enemyStacks.push_back(ap.attack.defender);

	ReachabilityData exchangeUnits = getExchangeUnits(ap, turn, targets, hb);

	if(exchangeUnits.units.empty())
	{
		return BattleScore();
	}

	auto exchangeBattle = std::make_shared<HypotheticBattle>(env.get(), hb);
	BattleExchangeVariant v;

	for(auto unit : exchangeUnits.units)
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

	auto melleeAttackers = ourStacks;

	vstd::removeDuplicates(melleeAttackers);
	vstd::erase_if(melleeAttackers, [&](const battle::Unit * u) -> bool
		{
			return cb->battleCanShoot(u);
		});

	bool canUseAp = true;

	for(auto activeUnit : exchangeUnits.units)
	{
		bool isOur = exchangeBattle->battleMatchOwner(ap.attack.attacker, activeUnit, true);
		battle::Units & attackerQueue = isOur ? ourStacks : enemyStacks;
		battle::Units & oppositeQueue = isOur ? enemyStacks : ourStacks;

		auto attacker = exchangeBattle->getForUpdate(activeUnit->unitId());

		if(!attacker->alive())
		{
#if BATTLE_TRACE_LEVEL>=1
			logAi->trace(	"Attacker is dead");
#endif

			continue;
		}

		auto targetUnit = ap.attack.defender;

		if(!isOur || !exchangeBattle->battleGetUnitByID(targetUnit->unitId())->alive())
		{
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

#if BATTLE_TRACE_LEVEL>=1
				logAi->trace("Best target selector %s->%s score = %2f", attacker->getDescription(), stackWithBonuses->getDescription(), score);
#endif

				return score;
			};

			auto unitsInOppositeQueueExceptInaccessible = oppositeQueue;

			vstd::erase_if(unitsInOppositeQueueExceptInaccessible, [&](const battle::Unit * u)->bool
				{
					return vstd::contains(exchangeUnits.shooters, u);
				});

			if(!unitsInOppositeQueueExceptInaccessible.empty())
			{
				targetUnit = *vstd::maxElementByFun(unitsInOppositeQueueExceptInaccessible, estimateAttack);
			}
			else
			{
				auto reachable = exchangeBattle->battleGetUnitsIf([&](const battle::Unit * u) -> bool
					{
						if(u->unitSide() == attacker->unitSide())
							return false;

						if(!exchangeBattle->getForUpdate(u->unitId())->alive())
							return false;

						return vstd::contains_if(reachabilityMap[u->getPosition()], [&](const battle::Unit * other) -> bool
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
		auto shooting = exchangeBattle->battleCanShoot(attacker.get());
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

	// avoid blocking path for stronger stack by weaker stack
	// the method checks if all stacks can be placed around enemy
	std::map<BattleHex, battle::Units> reachabilityMap;

	auto hexes = ap.attack.defender->getSurroundingHexes();

	for(auto hex : hexes)
		reachabilityMap[hex] = getOneTurnReachableUnits(turn, hex);

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Exchange score: enemy: %2f, our -%2f", v.getScore().enemyDamageReduce, v.getScore().ourDamageReduce);
#endif

	return v.getScore();
}

bool BattleExchangeEvaluator::canBeHitThisTurn(const AttackPossibility & ap)
{
	for(auto pos : ap.attack.attacker->getSurroundingHexes())
	{
		for(auto u : reachabilityMap[pos])
		{
			if(u->unitSide() != ap.attack.attacker->unitSide())
			{
				return true;
			}
		}
	}

	return false;
}

void BattleExchangeEvaluator::updateReachabilityMap(std::shared_ptr<HypotheticBattle> hb)
{
	const int TURN_DEPTH = 2;

	turnOrder.clear();

	hb->battleGetTurnOrder(turnOrder, std::numeric_limits<int>::max(), TURN_DEPTH);

	for(auto turn : turnOrder)
	{
		for(auto u : turn)
		{
			if(!vstd::contains(reachabilityCache, u->unitId()))
			{
				reachabilityCache[u->unitId()] = hb->getReachability(u);
			}
		}
	}

	for(BattleHex hex = BattleHex::TOP_LEFT; hex.isValid(); hex = hex + 1)
	{
		reachabilityMap[hex] = getOneTurnReachableUnits(0, hex);
	}
}

std::vector<const battle::Unit *> BattleExchangeEvaluator::getOneTurnReachableUnits(uint8_t turn, BattleHex hex)
{
	std::vector<const battle::Unit *> result;

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

			ReachabilityInfo unitReachability = vstd::getOrCompute(
				reachabilityCache,
				unit->unitId(),
				[&](ReachabilityInfo & data)
				{
					data = turnBattle.getReachability(unit);
				});

			bool reachable = unitReachability.distances[hex] <= radius;

			if(!reachable && unitReachability.accessibility[hex] == EAccessibility::ALIVE_STACK)
			{
				const battle::Unit * hexStack = cb->battleGetUnitByPos(hex);

				if(hexStack && cb->battleMatchOwner(unit, hexStack, false))
				{
					for(BattleHex neighbor : hex.neighbouringTiles())
					{
						reachable = unitReachability.distances[neighbor] <= radius;

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

// avoid blocking path for stronger stack by weaker stack
bool BattleExchangeEvaluator::checkPositionBlocksOurStacks(HypotheticBattle & hb, const battle::Unit * activeUnit, BattleHex position)
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

			for(BattleHex hex = BattleHex::TOP_LEFT; hex.isValid(); hex = hex + 1)
			{
				bool enemyUnit = false;
				bool reachable = unitReachability.distances[hex] <= unitSpeed;

				if(!reachable && unitReachability.accessibility[hex] == EAccessibility::ALIVE_STACK)
				{
					const battle::Unit * hexStack = turnBattle.battleGetUnitByPos(hex);

					if(hexStack && cb->battleMatchOwner(unit, hexStack, false))
					{
						enemyUnit = true;

						for(BattleHex neighbor : hex.neighbouringTiles())
						{
							reachable = unitReachability.distances[neighbor] <= unitSpeed;

							if(reachable) break;
						}
					}
				}

				if(!reachable && std::count(reachabilityMap[hex].begin(), reachabilityMap[hex].end(), unit) > 1)
				{
					blockingScore += ratio * (enemyUnit ? BLOCKING_OWN_ATTACK_PENALTY : BLOCKING_OWN_MOVE_PENALTY);
				}
			}
		}
	}

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Position %d, blocking score %f", position.hex, blockingScore);
#endif

	return blockingScore > BLOCKING_THRESHOLD;
}
