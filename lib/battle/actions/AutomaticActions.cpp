/*
 * AutomaticActions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AutomaticActions.h"

#include "MeleeAttackActions.h"

#include "../CBattleInfoCallback.h"
#include "../IBattleState.h"
#include "../../CCreatureHandler.h"
#include "../../CStack.h"
#include "../../callback/GameRandomizer.h"
#include "../../mapObjects/CGHeroInstance.h"

static double calculateTowerAttackValue(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * target)
{
	double unitValue = target->unitType()->getAIValue();
	double singleHpValue = unitValue / static_cast<double>(target->getMaxHealth());
	double fullHp = static_cast<double>(target->getTotalHealth());

	int distance = BattleHex::getDistance(attacker->getPosition(), target->getPosition());
	BattleAttackInfo attackInfo(attacker, target, distance, attacker->isShooter());
	DamageEstimation estimation = battle.calculateDmgRange(attackInfo);

	double avgDmg = (static_cast<double>(estimation.damage.max) + static_cast<double>(estimation.damage.min)) / 2.0;
	double realAvgDmg = std::min(fullHp, avgDmg);
	double avgUnitKilled = (static_cast<double>(estimation.kills.max) + static_cast<double>(estimation.kills.min)) / 2.0;

	return (realAvgDmg * singleHpValue) + (avgUnitKilled * unitValue);
}

/// Hero of the side of the unit takes control of the unit instead of the CPU, by the MANUAL_CONTROL bonus for the creature
static bool rollManualControl(const CBattleInfoCallback & battle, const CStack & unit, CreatureID creature, GameRandomizer & randomizer)
{
	const CGHeroInstance * owner = battle.battleGetOwnerHero(&unit);
	return owner && randomizer.rollCombatAbility(owner->id, owner->valOfBonuses(BonusType::MANUAL_CONTROL, BonusSubtypeID(creature)));
}

static std::optional<BattleAction> chooseBadMoraleAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer)
{
	int morale = unit.moraleVal();
	if(unit.hadMorale || unit.waited() || morale >= 0)
		return std::nullopt;

	ObjectInstanceID ownerArmy = battle.getBattle()->getSideArmy(unit.unitSide())->id;
	if(!randomizer.rollBadMorale(ownerArmy, -morale))
		return std::nullopt;

	return BattleAction::makeBadMorale(&unit);
}

static std::optional<BattleAction> chooseRangedUnitAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer)
{
	const CStack * next = &unit;

	if(!next->hasBonusOfType(BonusType::CPU_CONTROLLED) || (!battle.battleCanShoot(next) && next->isMeleeAttacker()))
		return std::nullopt;

	if(rollManualControl(battle, unit, next->unitType()->getId(), randomizer))
		return std::nullopt;

	const TStacks possibleTargets = battle.battleGetStacksIf([&next, &battle](const CStack * s)
	{
		return battle.battleMatchOwner(next, s) && s->isValidTarget() && battle.battleCanShoot(next, s->getPosition());
	});

	struct TargetInfo
	{
		bool insideTheWalls;
		bool canAttackNextTurn;
		bool isParalyzed;
		bool isMachine;
		double towerAttackValue;
		const CStack * stack;
	};

	const auto & getCanAttackNextTurn = [&battle] (const battle::Unit * unit)
	{
		if (battle.battleCanShoot(unit))
			return true;

		auto units = battle.battleAliveUnits();
		auto availableHexes = battle.battleGetAvailableHexes(unit, true);

		for (const auto * otherUnit : units)
		{
			if (battle.battleCanAttackUnit(unit, otherUnit))
				for (auto position : otherUnit->getHexes())
				{
					if (battle.battleCanAttackHex(availableHexes, unit, position))
						return true;
				}
		}
		return false;
	};

	std::vector<TargetInfo> targetsInfo;

	for (const CStack * possibleTarget : possibleTargets)
	{
		bool isMachine = possibleTarget->unitType()->warMachine != ArtifactID::NONE;
		bool isParalyzed = possibleTarget->hasBonusOfType(BonusType::NOT_ACTIVE) && !isMachine;
		const TargetInfo targetInfo =
		{
			battle.battleIsInsideWalls(possibleTarget->getPosition()),
			getCanAttackNextTurn(possibleTarget),
			isParalyzed,
			isMachine,
			calculateTowerAttackValue(battle, next, possibleTarget),
			possibleTarget
		};
		targetsInfo.push_back(targetInfo);
	}

	const auto & isBetterTarget = [](const TargetInfo & candidate, const TargetInfo & current)
	{
		if (candidate.isParalyzed != current.isParalyzed)
			return candidate.isParalyzed < current.isParalyzed;

		if (candidate.isMachine != current.isMachine)
			return candidate.isMachine < current.isMachine;

		if (candidate.canAttackNextTurn != current.canAttackNextTurn)
			return candidate.canAttackNextTurn > current.canAttackNextTurn;

		if (candidate.insideTheWalls != current.insideTheWalls)
			return candidate.insideTheWalls > current.insideTheWalls;

		return candidate.towerAttackValue > current.towerAttackValue;
	};

	const TargetInfo * target = nullptr;

	for(const auto & elem : targetsInfo)
	{
		if (target == nullptr || isBetterTarget(elem, *target))
			target = &elem;
	}

	if(target == nullptr)
		return BattleAction::makeNoAction(next);

	return BattleAction::makeShotAttack(next, target->stack);
}

static std::optional<BattleAction> chooseMeleeUnitAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer)
{
	struct TargetInfo
	{
		bool isMachine = false;
		bool isParalyzed = false;
		bool isReachable = false;
		BattleHex attackableFromHex = BattleHex::INVALID;
		double attackValue = -1.0;
		const CStack * stack = nullptr;

		bool isBetterThan(const TargetInfo & other) const
		{
			if (isParalyzed != other.isParalyzed) return isParalyzed < other.isParalyzed;
			if (isMachine != other.isMachine) return isMachine < other.isMachine;
			if (isReachable != other.isReachable) return isReachable > other.isReachable;
			return attackValue > other.attackValue;
		}
	};

	const CStack * actingStack = &unit;

	if (!actingStack->hasBonusOfType(BonusType::CPU_CONTROLLED) || battle.battleCanShoot(actingStack))
		return std::nullopt;

	if (rollManualControl(battle, unit, actingStack->unitType()->getId(), randomizer))
		return std::nullopt;

	ReachabilityInfo reachabilityCache = battle.getReachability(actingStack);

	const TStacks possibleTargets = battle.battleGetStacksIf([&actingStack, &battle](const CStack * s)
	{
		return battle.battleMatchOwner(actingStack, s) && s->isValidTarget()
		&& (!battle.isEnemyUnitWithinSpecifiedRange(actingStack->position, s, actingStack->getMovementRange())
		|| (battle.isEnemyUnitWithinSpecifiedRange(actingStack->position, s, actingStack->getMovementRange()) && !s->getAttackableHexes(actingStack).empty()));
	});

	TargetInfo bestTarget;

	for (const CStack * possibleTarget : possibleTargets)
	{
		bool isMachine = possibleTarget->unitType()->warMachine != ArtifactID::NONE;
		bool isParalyzed = possibleTarget->hasBonusOfType(BonusType::NOT_ACTIVE) && !isMachine;

		BattleHexArray attackableHexes;
		const auto availableHexes = battle.battleGetAvailableHexes(reachabilityCache, actingStack, true);

		for(const BattleHex & targetHex : possibleTarget->getHexes())
		{
			for(int direction = 0; direction < 8; ++direction)
			{
				for(const bool longWeapon : {false, true})
				{
					const BattleHex attackFromHex = battle.findAttackPosition(availableHexes, actingStack, targetHex, static_cast<BattleHex::EDir>(direction), longWeapon);
					if(attackFromHex.isValid())
						attackableHexes.insert(attackFromHex);
				}
			}
		}

		bool isReachable = !attackableHexes.empty();
		if(!isReachable)
			continue;

		BattleHex closestTargetAdjacentHex = std::ranges::min_element(attackableHexes, [&reachabilityCache](const BattleHex & lhs, const BattleHex & rhs)
		{
			return reachabilityCache.distances[lhs.toInt()] < reachabilityCache.distances[rhs.toInt()];
		})[0];

		TargetInfo currentTarget =
		{
			isMachine,
			isParalyzed,
			isReachable,
			closestTargetAdjacentHex,
			calculateTowerAttackValue(battle, actingStack, possibleTarget),
			possibleTarget
		};

		if (!bestTarget.stack || currentTarget.isBetterThan(bestTarget))
			bestTarget = currentTarget;
	}

	if(!bestTarget.stack)
		return BattleAction::makeNoAction(actingStack);

	if(bestTarget.isReachable)
	{
		const auto attack = MeleeAttackAction::buildAutomaticAttack(battle, *actingStack, *bestTarget.stack, bestTarget.attackableFromHex);
		return attack ? *attack : BattleAction::makeNoAction(actingStack);
	}

	if(actingStack->getMovementRange() > 0)
	{
		BattleHex intermediaryHex = battle.getClosestHexToTargetInRange(reachabilityCache, *actingStack, bestTarget.attackableFromHex);
		if(intermediaryHex == BattleHex::INVALID)
			return BattleAction::makeNoAction(actingStack);
		return BattleAction::makeMove(actingStack, intermediaryHex);
	}

	return BattleAction::makeNoAction(actingStack);
}

static std::optional<BattleAction> chooseCatapultAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer)
{
	if (!unit.isCatapult())
		return std::nullopt;

	if (battle.getAttackableWallParts().empty())
		return BattleAction::makeNoAction(&unit);

	if (rollManualControl(battle, unit, CreatureID(CreatureID::CATAPULT), randomizer))
		return std::nullopt;

	return BattleAction::makeCatapultShot(&unit);
}

static std::optional<BattleAction> chooseFirstAidTentAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer)
{
	if (!unit.isFirstAidTent())
		return std::nullopt;

	if (rollManualControl(battle, unit, CreatureID(CreatureID::FIRST_AID_TENT), randomizer))
		return std::nullopt;

	return BattleAction::makeHeal(&unit);
}

std::optional<BattleAction> AutomaticActions::chooseAction(const CBattleInfoCallback & battle, const CStack & unit, GameRandomizer & randomizer)
{
	if(auto action = chooseBadMoraleAction(battle, unit, randomizer))
		return action;

	if(unit.hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE))
		return chooseBerserkAction(battle, unit);

	for(const auto & choose : {chooseRangedUnitAction, chooseMeleeUnitAction, chooseCatapultAction, chooseFirstAidTentAction})
		if(auto action = choose(battle, unit, randomizer))
			return action;

	return std::nullopt;
}

BattleAction AutomaticActions::chooseBerserkAction(const CBattleInfoCallback & battle, const battle::Unit & unit)
{
	const battle::Unit * berserker = &unit;
	logGlobal->trace("Handle Berserk effect");
	const BattleAction noAction = BattleAction::makeNoAction(berserker);

	auto targets = battle.battleGetUnitsIf([&berserker](const battle::Unit * u)
	{
		return u->isValidTarget(false) && !u->isInvincible() && u->unitId() != berserker->unitId();
	});

	if (targets.empty())
	{
		logGlobal->trace("No attackable target found for berserk unit");
		return noAction;
	}

	const auto nearestTarget = std::ranges::min_element(targets, [&berserker](const battle::Unit * lhs, const battle::Unit * rhs)
	{
		return BattleHex::getDistance(berserker->getPosition(), lhs->getPosition()) < BattleHex::getDistance(berserker->getPosition(), rhs->getPosition());
	})[0];

	// a shooter that can not shoot its nearest target, e.g. Steel Elves next to it or out of limited range, attacks in melee
	if (battle.battleCanShoot(berserker, nearestTarget->getPosition()))
		return BattleAction::makeShotAttack(berserker, nearestTarget);

	struct TargetData
	{
		const battle::Unit * target;
		BattleHex closestAttackableHex;
		uint32_t distance;
	};

	auto cache = battle.getReachability(berserker);
	std::vector<TargetData> targetData;
	targetData.reserve(targets.size());
	for (const battle::Unit * uTarget : targets)
	{
		BattleHexArray attackableHexes = uTarget->getAttackableHexes(berserker);
		// a double-wide berserker may have no room to stand next to the target
		if (attackableHexes.empty())
			continue;

		auto closestAttackableHex = std::ranges::min_element(attackableHexes, [&cache](const BattleHex & lhs, const BattleHex & rhs)
		{
			return cache.distances[lhs.toInt()] < cache.distances[rhs.toInt()];
		})[0];
		targetData.push_back({uTarget, closestAttackableHex, cache.distances[closestAttackableHex.toInt()]});
	}

	if (targetData.empty())
	{
		logGlobal->trace("No room to attack any target");
		return noAction;
	}

	auto closestUnit = std::ranges::min_element(targetData, [](const TargetData & lhs, const TargetData & rhs)
	{
		return lhs.distance < rhs.distance;
	})[0];

	if (closestUnit.distance <= berserker->getMovementRange())
		return MeleeAttackAction::buildAutomaticAttack(battle, *berserker, *closestUnit.target, closestUnit.closestAttackableHex).value_or(noAction);

	if (closestUnit.distance != ReachabilityInfo::INFINITE_DIST && berserker->getMovementRange() > 0)
	{
		const BattleHex hex = battle.getClosestHexToTargetInRange(cache, *berserker, closestUnit.closestAttackableHex);
		return hex.isValid() ? BattleAction::makeMove(berserker, hex) : noAction;
	}

	logGlobal->trace("No target found or unit cannot move");
	return noAction;
}
