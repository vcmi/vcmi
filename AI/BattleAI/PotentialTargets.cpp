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

PotentialTargets::PotentialTargets(const CStack *attacker, const HypotheticChangesToBattleState &state /*= HypotheticChangesToBattleState()*/)
{
	auto dists = getCbc()->battleGetDistances(attacker);
	auto avHexes = getCbc()->battleGetAvailableHexes(attacker, false);

	for(const CStack *enemy : getCbc()->battleGetStacks())
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

		if(getCbc()->battleCanShoot(attacker, enemy->position))
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



int PotentialTargets::bestActionValue() const
{
	if(possibleAttacks.empty())
		return 0;

	return bestAction().attackValue();
}

AttackPossibility PotentialTargets::bestAction() const
{
	if(possibleAttacks.empty())
		throw std::runtime_error("No best action, since we don't have any actions");

	return *vstd::maxElementByFun(possibleAttacks, [](const AttackPossibility &ap) { return ap.attackValue(); } );
}
