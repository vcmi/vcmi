/*
 * PotentialTargets.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PotentialTargets.h"
#include "../../lib/CStack.h"//todo: remove

PotentialTargets::PotentialTargets(const battle::Unit * attacker, const HypotheticBattle * state)
{
	auto attackerInfo = state->battleGetUnitByID(attacker->unitId());
	auto reachability = state->getReachability(attackerInfo);
	auto avHexes = state->battleGetAvailableHexes(reachability, attackerInfo);

	//FIXME: this should part of battleGetAvailableHexes
	bool forceTarget = false;
	const battle::Unit * forcedTarget = nullptr;
	BattleHex forcedHex;

	if(attackerInfo->hasBonusOfType(Bonus::ATTACKS_NEAREST_CREATURE))
	{
		forceTarget = true;
		auto nearest = state->getNearestStack(attackerInfo);

		if(nearest.first != nullptr)
		{
			forcedTarget = nearest.first;
			forcedHex = nearest.second;
		}
	}

	auto aliveUnits = state->battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->isValidTarget() && unit->unitId() != attackerInfo->unitId();
	});

	for(auto defender : aliveUnits)
	{
		if(!forceTarget && !state->battleMatchOwner(attackerInfo, defender))
			continue;

		auto GenerateAttackInfo = [&](bool shooting, BattleHex hex) -> AttackPossibility
		{
			auto bai = BattleAttackInfo(attackerInfo, defender, shooting);

			if(hex.isValid() && !shooting)
				bai.chargedFields = reachability.distances[hex];

			return AttackPossibility::evaluate(bai, hex, state);
		};

		if(forceTarget)
		{
			if(forcedTarget && defender->unitId() == forcedTarget->unitId())
				possibleAttacks.push_back(GenerateAttackInfo(false, forcedHex));
			else
				unreachableEnemies.push_back(defender);
		}
		else if(state->battleCanShoot(attackerInfo, defender->getPosition()))
		{
			possibleAttacks.push_back(GenerateAttackInfo(true, BattleHex::INVALID));
		}
		else
		{
			for(BattleHex hex : avHexes)
			{
				if(!CStack::isMeleeAttackPossible(attackerInfo, defender, hex))
					continue;

				auto bai = GenerateAttackInfo(false, hex);
				if(!bai.affectedUnits.empty())
					possibleAttacks.push_back(bai);
			}

			if(!vstd::contains_if(possibleAttacks, [=](const AttackPossibility & pa) { return pa.attack.defender->unitId() == defender->unitId(); }))
				unreachableEnemies.push_back(defender);
		}
	}

	boost::sort(possibleAttacks, [](const AttackPossibility & lhs, const AttackPossibility & rhs) -> bool
	{
		if(lhs.collateralDamage > rhs.collateralDamage)
			return false;
		if(lhs.collateralDamage < rhs.collateralDamage)
			return true;
		return (lhs.damageDealt + lhs.shootersBlockedDmg - lhs.damageReceived > rhs.damageDealt + rhs.shootersBlockedDmg - rhs.damageReceived);
	});

	if (!possibleAttacks.empty())
	{
		auto & bestAp = possibleAttacks[0];

		logGlobal->info("Battle AI best: %s -> %s at %d from %d, affects %d units: %lld %lld %lld %lld",
			bestAp.attack.attacker->unitType()->identifier,
			state->battleGetUnitByPos(bestAp.dest)->unitType()->identifier,
			(int)bestAp.dest, (int)bestAp.from, (int)bestAp.affectedUnits.size(),
			bestAp.damageDealt, bestAp.damageReceived, bestAp.collateralDamage, bestAp.shootersBlockedDmg);
	}
}

int64_t PotentialTargets::bestActionValue() const
{
	if(possibleAttacks.empty())
		return 0;
	return bestAction().attackValue();
}

const AttackPossibility & PotentialTargets::bestAction() const
{
	if(possibleAttacks.empty())
		throw std::runtime_error("No best action, since we don't have any actions");
	return possibleAttacks.at(0);
	//return *vstd::maxElementByFun(possibleAttacks, [](const AttackPossibility &ap) { return ap.attackValue(); } );
}
