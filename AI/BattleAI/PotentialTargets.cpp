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
#include "../../lib/battle/actions/AutomaticActions.h"
#include "../../lib/mapObjects/CGTownInstance.h"

PotentialTargets::PotentialTargets(
	const battle::Unit * attacker,
	DamageCache & damageCache,
	std::shared_ptr<HypotheticBattle> state)
{
	auto attackerInfo = state->battleGetUnitByID(attacker->unitId());
	auto reachability = state->getReachability(attackerInfo);
	auto avHexes = state->battleGetAvailableHexes(reachability, attackerInfo, false);

	//FIXME: this should part of battleGetAvailableHexes
	bool isBerserk = attackerInfo->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE);
	// the unit that a berserk attacker attacks, and the hex it attacks from, or nullptr if it attacks no unit
	const battle::Unit * berserkTarget = nullptr;
	BattleHex berserkAttackFrom = BattleHex::INVALID;
	bool berserkShoots = false;

	if(isBerserk)
	{
		const BattleAction forcedAction = AutomaticActions::chooseBerserkAction(*state, *attackerInfo);
		const bool melee = forcedAction.actionType == EActionType::WALK_AND_ATTACK
			|| forcedAction.actionType == EActionType::WALK_AND_ATTACK_NO_RETURN
			|| forcedAction.actionType == EActionType::LONG_WEAPON_ATTACK;
		berserkShoots = forcedAction.actionType == EActionType::SHOOT;

		if(melee || berserkShoots)
		{
			const battle::Target target = forcedAction.getTarget(state.get());
			const battle::Destination & attacked = target.back();
			berserkTarget = attacked.unitValue ? attacked.unitValue : state->battleGetUnitByPos(attacked.hexValue, true);
			if(melee)
				berserkAttackFrom = target.front().hexValue;
		}
	}

	auto aliveUnits = state->battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->isValidTarget() && !unit->isInvincible() && unit->unitId() != attackerInfo->unitId();
	});

	for(auto defender : aliveUnits)
	{
		if(!isBerserk && !state->battleMatchOwner(attackerInfo, defender))
			continue;

		auto GenerateAttackInfo = [&](bool shooting, const BattleHex & hex) -> AttackPossibility
		{
			int distance = hex.isValid() ? reachability.distances[hex.toInt()] : 0;
			auto bai = BattleAttackInfo(attackerInfo, defender, distance, shooting);

			return AttackPossibility::evaluate(bai, hex, damageCache, state);
		};

		if(isBerserk)
		{
			if (berserkTarget && defender->unitId() == berserkTarget->unitId())
			{
				possibleAttacks.push_back(GenerateAttackInfo(berserkShoots, berserkAttackFrom));
			}
			else
			{
				unreachableEnemies.push_back(defender);
			}
		}
		else if(state->battleCanShoot(attackerInfo, defender->getPosition()))
		{
			possibleAttacks.push_back(GenerateAttackInfo(true, BattleHex::INVALID));
		}
		else
		{
			// a blocked war machine, e.g. Ballista, can neither shoot nor fight in melee
			for(const BattleHex & hex : avHexes)
			{
				if(!attackerInfo->isMeleeAttacker() || !state->isMeleeAttackPossible(attackerInfo, defender, hex))
					continue;

				auto bai = GenerateAttackInfo(false, hex);
				if(!bai.affectedUnits.empty())
					possibleAttacks.push_back(bai);
			}

			if(!vstd::contains_if(possibleAttacks, [=](const AttackPossibility & pa) { return pa.attack.defender->unitId() == defender->unitId(); }))
				unreachableEnemies.push_back(defender);
		}
	}

	std::ranges::sort(possibleAttacks, [](const AttackPossibility & lhs, const AttackPossibility & rhs) -> bool
	{
		return lhs.damageDiff() > rhs.damageDiff();
	});
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

	return possibleAttacks.front();
}
