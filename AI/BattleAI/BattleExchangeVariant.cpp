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
{
	value = 0;
	isRetalitated = false;
}

MoveTarget::MoveTarget()
	: positions()
{
	score = EvaluationResult::INEFFECTIVE_SCORE;
}

int64_t BattleExchangeVariant::trackAttack(const AttackPossibility & ap, HypotheticBattle & state)
{
	auto affectedUnits = ap.affectedUnits;

	affectedUnits.push_back(ap.attackerState);

	for(auto affectedUnit : affectedUnits)
	{
		auto unitToUpdate = state.getForUpdate(affectedUnit->unitId());

		unitToUpdate->health = affectedUnit->health;
		unitToUpdate->shots = affectedUnit->shots;
		unitToUpdate->counterAttacks = affectedUnit->counterAttacks;
		unitToUpdate->movedThisRound = affectedUnit->movedThisRound;
	}

	auto attackValue = ap.attackValue();

	dpsScore += attackValue;

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace(
		"%s -> %s, ap attack, %s, dps: %lld, score: %lld",
		ap.attack.attacker->getDescription(),
		ap.attack.defender->getDescription(),
		ap.attack.shooting ? "shot" : "mellee",
		ap.damageDealt,
		attackValue);
#endif

	return attackValue;
}

int64_t BattleExchangeVariant::trackAttack(
	std::shared_ptr<StackWithBonuses> attacker,
	std::shared_ptr<StackWithBonuses> defender,
	bool shooting,
	bool isOurAttack,
	const CBattleInfoCallback & cb,
	bool evaluateOnly)
{
	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type()(Bonus::BLOCKS_RETALIATION);
	const bool counterAttacksBlocked = attacker->hasBonus(selectorBlocksRetaliation, cachingStringBlocksRetaliation);

	DamageEstimation retaliation;
	// FIXME: provide distance info for Jousting bonus
	BattleAttackInfo bai(attacker.get(), defender.get(), 0, shooting);

	if(shooting)
	{
		bai.attackerPos.setXY(8, 5);
	}

	auto attack = cb.battleEstimateDamage(bai, &retaliation);
	int64_t attackDamage = (attack.damage.min + attack.damage.max) / 2;
	int64_t defenderDamageReduce = AttackPossibility::calculateDamageReduce(attacker.get(), defender.get(), attackDamage, cb);
	int64_t attackerDamageReduce = 0;

	if(!evaluateOnly)
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace(
			"%s -> %s, normal attack, %s, dps: %lld, %lld",
			attacker->getDescription(),
			defender->getDescription(),
			shooting ? "shot" : "mellee",
			attackDamage,
			defenderDamageReduce);
#endif

		if(isOurAttack)
		{
			dpsScore += defenderDamageReduce;
			attackerValue[attacker->unitId()].value += defenderDamageReduce;
		}
		else
			dpsScore -= defenderDamageReduce;

		defender->damage(attackDamage);
		attacker->afterAttack(shooting, false);
	}

	if(defender->alive() && defender->ableToRetaliate() && !counterAttacksBlocked && !shooting)
	{
		if(retaliation.damage.max != 0)
		{
			auto retaliationDamage = (retaliation.damage.min + retaliation.damage.max) / 2;
			attackerDamageReduce = AttackPossibility::calculateDamageReduce(defender.get(), attacker.get(), retaliationDamage, cb);

			if(!evaluateOnly)
			{
#if BATTLE_TRACE_LEVEL>=1
				logAi->trace(
					"%s -> %s, retaliation, dps: %lld, %lld",
					defender->getDescription(),
					attacker->getDescription(),
					retaliationDamage,
					attackerDamageReduce);
#endif

				if(isOurAttack)
				{
					dpsScore -= attackerDamageReduce;
					attackerValue[attacker->unitId()].isRetalitated = true;
				}
				else
				{
					dpsScore += attackerDamageReduce;
					attackerValue[defender->unitId()].value += attackerDamageReduce;
				}

				attacker->damage(retaliationDamage);
				defender->afterAttack(false, true);
			}
		}
	}

	auto score = defenderDamageReduce - attackerDamageReduce;

#if BATTLE_TRACE_LEVEL>=1
	if(!score)
	{
		logAi->trace("Attack has zero score d:%lld a:%lld", defenderDamageReduce, attackerDamageReduce);
	}
#endif

	return score;
}

EvaluationResult BattleExchangeEvaluator::findBestTarget(const battle::Unit * activeStack, PotentialTargets & targets, HypotheticBattle & hb)
{
	EvaluationResult result(targets.bestAction());

	updateReachabilityMap(hb);

	for(auto & ap : targets.possibleAttacks)
	{
		int64_t score = calculateExchange(ap, targets, hb);

		if(score > result.score)
		{
			result.score = score;
			result.bestAttack = ap;
		}
	}

	if(!activeStack->waited())
	{
#if BATTLE_TRACE_LEVEL>=1
		logAi->trace("Evaluating waited attack for %s", activeStack->getDescription());
#endif

		hb.getForUpdate(activeStack->unitId())->waiting = true;
		hb.getForUpdate(activeStack->unitId())->waitedThisTurn = true;

		updateReachabilityMap(hb);

		for(auto & ap : targets.possibleAttacks)
		{
			int64_t score = calculateExchange(ap, targets, hb);

			if(score > result.score)
			{
				result.score = score;
				result.bestAttack = ap;
				result.wait = true;
			}
		}
	}

	return result;
}

MoveTarget BattleExchangeEvaluator::findMoveTowardsUnreachable(const battle::Unit * activeStack, PotentialTargets & targets, HypotheticBattle & hb)
{
	MoveTarget result;
	BattleExchangeVariant ev;

	if(targets.unreachableEnemies.empty())
		return result;

	auto speed = activeStack->Speed();

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

		for(auto hex : hexes)
		{
			// FIXME: provide distance info for Jousting bonus
			auto bai = BattleAttackInfo(activeStack, closestStack, 0, cb->battleCanShoot(activeStack));
			auto attack = AttackPossibility::evaluate(bai, hex, hb);

			attack.shootersBlockedDmg = 0; // we do not want to count on it, it is not for sure

			auto score = calculateExchange(attack, targets, hb) / turnsToRich;

			if(result.score < score)
			{
				result.score = score;
				result.positions = closestStack->getAttackableHexes(activeStack);
			}
		}
	}

	return result;
}

std::vector<const battle::Unit *> BattleExchangeEvaluator::getAdjacentUnits(const battle::Unit * blockerUnit)
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
			auto neighbor = cb->battleGetStackByPos(hex);

			if(neighbor && neighbor->unitSide() == stack->unitSide() && !vstd::contains(checkedStacks, neighbor))
			{
				queue.push(neighbor);
				checkedStacks.push_back(neighbor);
			}
		}
	}

	return checkedStacks;
}

std::vector<const battle::Unit *> BattleExchangeEvaluator::getExchangeUnits(
	const AttackPossibility & ap,
	PotentialTargets & targets,
	HypotheticBattle & hb)
{
	auto hexes = ap.attack.defender->getHexes();

	if(!ap.attack.shooting) hexes.push_back(ap.from);

	std::vector<const battle::Unit *> exchangeUnits;
	std::vector<const battle::Unit *> allReachableUnits;

	for(auto hex : hexes)
	{
		vstd::concatenate(allReachableUnits, reachabilityMap[hex]);
	}

	vstd::removeDuplicates(allReachableUnits);

	auto copy = allReachableUnits;
	for(auto unit : copy)
	{
		for(auto adjacentUnit : getAdjacentUnits(unit))
		{
			auto unitWithBonuses = hb.battleGetUnitByID(adjacentUnit->unitId());

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

		return exchangeUnits;
	}

	for(int turn = 0; turn < turnOrder.size(); turn++)
	{
		for(auto unit : turnOrder[turn])
		{
			if(vstd::contains(allReachableUnits, unit))
				exchangeUnits.push_back(unit);
		}
	}

	return exchangeUnits;
}

int64_t BattleExchangeEvaluator::calculateExchange(
	const AttackPossibility & ap,
	PotentialTargets & targets,
	HypotheticBattle & hb)
{
#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Battle exchange at %lld", ap.attack.shooting ? ap.dest : ap.from);
#endif

	if(cb->battleGetMySide() == BattlePerspective::LEFT_SIDE
		&& cb->battleGetGateState() == EGateState::BLOCKED
		&& ap.attack.defender->coversPos(ESiegeHex::GATE_BRIDGE))
	{
		return EvaluationResult::INEFFECTIVE_SCORE;
	}

	std::vector<const battle::Unit *> ourStacks;
	std::vector<const battle::Unit *> enemyStacks;

	enemyStacks.push_back(ap.attack.defender);

	std::vector<const battle::Unit *> exchangeUnits = getExchangeUnits(ap, targets, hb);

	if(exchangeUnits.empty())
	{
		return 0;
	}

	HypotheticBattle exchangeBattle(env.get(), cb);
	BattleExchangeVariant v;
	auto melleeAttackers = ourStacks;

	vstd::removeDuplicates(melleeAttackers);
	vstd::erase_if(melleeAttackers, [&](const battle::Unit * u) -> bool
		{
			return !cb->battleCanShoot(u);
		});

	for(auto unit : exchangeUnits)
	{
		bool isOur = cb->battleMatchOwner(ap.attack.attacker, unit, true);
		auto & attackerQueue = isOur ? ourStacks : enemyStacks;

		if(!vstd::contains(attackerQueue, unit))
		{
			attackerQueue.push_back(unit);
		}
	}

	bool canUseAp = true;

	for(auto activeUnit : exchangeUnits)
	{
		bool isOur = cb->battleMatchOwner(ap.attack.attacker, activeUnit, true);
		battle::Units & attackerQueue = isOur ? ourStacks : enemyStacks;
		battle::Units & oppositeQueue = isOur ? enemyStacks : ourStacks;

		auto attacker = exchangeBattle.getForUpdate(activeUnit->unitId());

		if(!attacker->alive())
		{
#if BATTLE_TRACE_LEVEL>=1
			logAi->trace(	"Attacker is dead");
#endif

			continue;
		}

		auto targetUnit = ap.attack.defender;

		if(!isOur || !exchangeBattle.getForUpdate(targetUnit->unitId())->alive())
		{
			auto estimateAttack = [&](const battle::Unit * u) -> int64_t
			{
				auto stackWithBonuses = exchangeBattle.getForUpdate(u->unitId());
				auto score = v.trackAttack(
					attacker,
					stackWithBonuses,
					exchangeBattle.battleCanShoot(stackWithBonuses.get()),
					isOur,
					*cb,
					true);

#if BATTLE_TRACE_LEVEL>=1
				logAi->trace("Best target selector %s->%s score = %lld", attacker->getDescription(), u->getDescription(), score);
#endif

				return score;
			};

			if(!oppositeQueue.empty())
			{
				targetUnit = *vstd::maxElementByFun(oppositeQueue, estimateAttack);
			}
			else
			{
				auto reachable = exchangeBattle.battleGetUnitsIf([&](const battle::Unit * u) -> bool
					{
						if(!u->alive() || u->unitSide() == attacker->unitSide())
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

		auto defender = exchangeBattle.getForUpdate(targetUnit->unitId());
		auto shooting = cb->battleCanShoot(attacker.get());
		const int totalAttacks = attacker->getTotalAttacks(shooting);

		if(canUseAp && activeUnit == ap.attack.attacker && targetUnit == ap.attack.defender)
		{
			v.trackAttack(ap, exchangeBattle);
		}
		else
		{
			for(int i = 0; i < totalAttacks; i++)
			{
				v.trackAttack(attacker, defender, shooting, isOur, exchangeBattle);

				if(!attacker->alive() || !defender->alive())
					break;
			}
		}

		canUseAp = false;

		vstd::erase_if(attackerQueue, [&](const battle::Unit * u) -> bool
			{
				return !exchangeBattle.getForUpdate(u->unitId())->alive();
			});

		vstd::erase_if(oppositeQueue, [&](const battle::Unit * u) -> bool
			{
				return !exchangeBattle.getForUpdate(u->unitId())->alive();
			});
	}

	// avoid blocking path for stronger stack by weaker stack
	// the method checks if all stacks can be placed around enemy
	v.adjustPositions(melleeAttackers, ap, reachabilityMap);

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("Exchange score: %lld", v.getScore());
#endif

	return v.getScore();
}

void BattleExchangeVariant::adjustPositions(
	std::vector<const battle::Unit*> attackers,
	const AttackPossibility & ap,
	std::map<BattleHex, battle::Units> & reachabilityMap)
{
	auto hexes = ap.attack.defender->getSurroundingHexes();

	boost::sort(attackers, [&](const battle::Unit * u1, const battle::Unit * u2) -> bool
		{
			if(attackerValue[u1->unitId()].isRetalitated && !attackerValue[u2->unitId()].isRetalitated)
				return true;

			if(attackerValue[u2->unitId()].isRetalitated && !attackerValue[u1->unitId()].isRetalitated)
				return false;

			return attackerValue[u1->unitId()].value > attackerValue[u2->unitId()].value;
		});

	if(!ap.attack.shooting)
	{
		vstd::erase_if_present(hexes, ap.from);
		vstd::erase_if_present(hexes, ap.attack.attacker->occupiedHex(ap.attack.attackerPos));
	}

	int64_t notRealizedDamage = 0;

	for(auto unit : attackers)
	{
		if(unit->unitId() == ap.attack.attacker->unitId())
			continue;

		if(!vstd::contains_if(hexes, [&](BattleHex h) -> bool
			{
				return vstd::contains(reachabilityMap[h], unit);
			}))
		{
			notRealizedDamage += attackerValue[unit->unitId()].value;
			continue;
		}

		auto desiredPosition = vstd::minElementByFun(hexes, [&](BattleHex h) -> int64_t
			{
				auto score = vstd::contains(reachabilityMap[h], unit)
					? reachabilityMap[h].size()
					: 0;

				if(unit->doubleWide())
				{
					auto backHex = unit->occupiedHex(h);

					if(vstd::contains(hexes, backHex))
						score += reachabilityMap[backHex].size();
				}

				return score;
			});

		hexes.erase(desiredPosition);
	}

	if(notRealizedDamage > ap.attackValue() && notRealizedDamage > attackerValue[ap.attack.attacker->unitId()].value)
	{
		dpsScore = EvaluationResult::INEFFECTIVE_SCORE;
	}
}

void BattleExchangeEvaluator::updateReachabilityMap(HypotheticBattle & hb)
{
	const int TURN_DEPTH = 2;

	turnOrder.clear();
	
	hb.battleGetTurnOrder(turnOrder, std::numeric_limits<int>::max(), TURN_DEPTH);
	reachabilityMap.clear();

	for(int turn = 0; turn < turnOrder.size(); turn++)
	{
		auto & turnQueue = turnOrder[turn];
		HypotheticBattle turnBattle(env.get(), cb);

		for(const battle::Unit * unit : turnQueue)
		{
			if(turnBattle.battleCanShoot(unit))
			{
				for(BattleHex hex = BattleHex::TOP_LEFT; hex.isValid(); hex = hex + 1)
				{
					reachabilityMap[hex].push_back(unit);
				}

				continue;
			}

			auto unitReachability = turnBattle.getReachability(unit);

			for(BattleHex hex = BattleHex::TOP_LEFT; hex.isValid(); hex = hex + 1)
			{
				bool reachable = unitReachability.distances[hex] <= unit->Speed(turn);

				if(!reachable && unitReachability.accessibility[hex] == EAccessibility::ALIVE_STACK)
				{
					const battle::Unit * hexStack = cb->battleGetUnitByPos(hex);

					if(hexStack && cb->battleMatchOwner(unit, hexStack, false))
					{
						for(BattleHex neighbor : hex.neighbouringTiles())
						{
							reachable = unitReachability.distances[neighbor] <= unit->Speed(turn);

							if(reachable) break;
						}
					}
				}

				if(reachable)
				{
					reachabilityMap[hex].push_back(unit);
				}
			}
		}
	}
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
			auto ratio = blockedUnitDamage / (blockedUnitDamage + activeUnitDamage);

			auto unitReachability = turnBattle.getReachability(unit);

			for(BattleHex hex = BattleHex::TOP_LEFT; hex.isValid(); hex = hex + 1)
			{
				bool enemyUnit = false;
				bool reachable = unitReachability.distances[hex] <= unit->Speed(turn);

				if(!reachable && unitReachability.accessibility[hex] == EAccessibility::ALIVE_STACK)
				{
					const battle::Unit * hexStack = turnBattle.battleGetUnitByPos(hex);

					if(hexStack && cb->battleMatchOwner(unit, hexStack, false))
					{
						enemyUnit = true;

						for(BattleHex neighbor : hex.neighbouringTiles())
						{
							reachable = unitReachability.distances[neighbor] <= unit->Speed(turn);

							if(reachable) break;
						}
					}
				}

				if(!reachable && vstd::contains(reachabilityMap[hex], unit))
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
