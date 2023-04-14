/*
 * AttackPossibility.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AttackPossibility.h"
#include "../../lib/CStack.h" // TODO: remove
                              // Eventually only IBattleInfoCallback and battle::Unit should be used, 
                              // CUnitState should be private and CStack should be removed completely

uint64_t averageDmg(const DamageRange & range)
{
	return (range.min + range.max) / 2;
}

AttackPossibility::AttackPossibility(BattleHex from, BattleHex dest, const BattleAttackInfo & attack)
	: from(from), dest(dest), attack(attack)
{
}

int64_t AttackPossibility::damageDiff() const
{
	return defenderDamageReduce - attackerDamageReduce - collateralDamageReduce + shootersBlockedDmg;
}

int64_t AttackPossibility::attackValue() const
{
	return damageDiff();
}

/// <summary>
/// How enemy damage will be reduced by this attack
/// Half bounty for kill, half for making damage equal to enemy health
/// Bounty - the killed creature average damage calculated against attacker
/// </summary>
int64_t AttackPossibility::calculateDamageReduce(
	const battle::Unit * attacker,
	const battle::Unit * defender,
	uint64_t damageDealt,
	const CBattleInfoCallback & cb)
{
	const float HEALTH_BOUNTY = 0.5;
	const float KILL_BOUNTY = 1.0 - HEALTH_BOUNTY;

	vstd::amin(damageDealt, defender->getAvailableHealth());

	// FIXME: provide distance info for Jousting bonus
	auto enemyDamageBeforeAttack = cb.battleEstimateDamage(defender, attacker, 0);
	auto enemiesKilled = damageDealt / defender->MaxHealth() + (damageDealt % defender->MaxHealth() >= defender->getFirstHPleft() ? 1 : 0);
	auto enemyDamage = averageDmg(enemyDamageBeforeAttack.damage);
	auto damagePerEnemy = enemyDamage / (double)defender->getCount();

	return (int64_t)(damagePerEnemy * (enemiesKilled * KILL_BOUNTY + damageDealt * HEALTH_BOUNTY / (double)defender->MaxHealth()));
}

int64_t AttackPossibility::evaluateBlockedShootersDmg(const BattleAttackInfo & attackInfo, BattleHex hex, const HypotheticBattle & state)
{
	int64_t res = 0;

	if(attackInfo.shooting)
		return 0;

	auto attacker = attackInfo.attacker;
	auto hexes = attacker->getSurroundingHexes(hex);
	for(BattleHex tile : hexes)
	{
		auto st = state.battleGetUnitByPos(tile, true);
		if(!st || !state.battleMatchOwner(st, attacker))
			continue;
		if(!state.battleCanShoot(st))
			continue;

		// FIXME: provide distance info for Jousting bonus
		BattleAttackInfo rangeAttackInfo(st, attacker, 0, true);
		rangeAttackInfo.defenderPos = hex;

		BattleAttackInfo meleeAttackInfo(st, attacker, 0, false);
		meleeAttackInfo.defenderPos = hex;

		auto rangeDmg = state.battleEstimateDamage(rangeAttackInfo);
		auto meleeDmg = state.battleEstimateDamage(meleeAttackInfo);

		int64_t gain = averageDmg(rangeDmg.damage) - averageDmg(meleeDmg.damage) + 1;
		res += gain;
	}

	return res;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo & attackInfo, BattleHex hex, const HypotheticBattle & state)
{
	auto attacker = attackInfo.attacker;
	auto defender = attackInfo.defender;
	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type()(Bonus::BLOCKS_RETALIATION);
	const auto attackerSide = state.playerToSide(state.battleGetOwner(attacker));
	const bool counterAttacksBlocked = attacker->hasBonus(selectorBlocksRetaliation, cachingStringBlocksRetaliation);

	AttackPossibility bestAp(hex, BattleHex::INVALID, attackInfo);

	std::vector<BattleHex> defenderHex;
	if(attackInfo.shooting)
		defenderHex = defender->getHexes();
	else
		defenderHex = CStack::meleeAttackHexes(attacker, defender, hex);

	for(BattleHex defHex : defenderHex)
	{
		if(defHex == hex) // should be impossible but check anyway
			continue;

		AttackPossibility ap(hex, defHex, attackInfo);
		ap.attackerState = attacker->acquireState();
		ap.shootersBlockedDmg = bestAp.shootersBlockedDmg;

		const int totalAttacks = ap.attackerState->getTotalAttacks(attackInfo.shooting);

		if (!attackInfo.shooting)
			ap.attackerState->setPosition(hex);

		std::vector<const battle::Unit*> units;

		if (attackInfo.shooting)
			units = state.getAttackedBattleUnits(attacker, defHex, true, BattleHex::INVALID);
		else
			units = state.getAttackedBattleUnits(attacker, defHex, false, hex);

		// ensure the defender is also affected
		bool addDefender = true;
		for(auto unit : units)
		{
			if (unit->unitId() == defender->unitId())
			{
				addDefender = false;
				break;
			}
		}

		if(addDefender)
			units.push_back(defender);

		for(auto u : units)
		{
			if(!ap.attackerState->alive())
				break;

			auto defenderState = u->acquireState();
			ap.affectedUnits.push_back(defenderState);

			for(int i = 0; i < totalAttacks; i++)
			{
				int64_t damageDealt, damageReceived, defenderDamageReduce, attackerDamageReduce;

				DamageEstimation retaliation;
				auto attackDmg = state.battleEstimateDamage(ap.attack, &retaliation);

				vstd::amin(attackDmg.damage.min, defenderState->getAvailableHealth());
				vstd::amin(attackDmg.damage.max, defenderState->getAvailableHealth());

				vstd::amin(retaliation.damage.min, ap.attackerState->getAvailableHealth());
				vstd::amin(retaliation.damage.max, ap.attackerState->getAvailableHealth());

				damageDealt = averageDmg(attackDmg.damage);
				defenderDamageReduce = calculateDamageReduce(attacker, defender, damageDealt, state);
				ap.attackerState->afterAttack(attackInfo.shooting, false);

				//FIXME: use ranged retaliation
				damageReceived = 0;
				attackerDamageReduce = 0;

				if (!attackInfo.shooting && defenderState->ableToRetaliate() && !counterAttacksBlocked)
				{
					damageReceived = averageDmg(retaliation.damage);
					attackerDamageReduce = calculateDamageReduce(defender, attacker, damageReceived, state);
					defenderState->afterAttack(attackInfo.shooting, true);
				}

				bool isEnemy = state.battleMatchOwner(attacker, u);

				// this includes enemy units as well as attacker units under enemy's mind control
				if(isEnemy)
					ap.defenderDamageReduce += defenderDamageReduce;

				// damaging attacker's units (even those under enemy's mind control) is considered friendly fire
				if(attackerSide == u->unitSide())
					ap.collateralDamageReduce += defenderDamageReduce;

				if(u->unitId() == defender->unitId() || 
					(!attackInfo.shooting && CStack::isMeleeAttackPossible(u, attacker, hex)))
				{
					//FIXME: handle RANGED_RETALIATION ?
					ap.attackerDamageReduce += attackerDamageReduce;
				}

				ap.attackerState->damage(damageReceived);
				defenderState->damage(damageDealt);

				if (!ap.attackerState->alive() || !defenderState->alive())
					break;
			}
		}

		if(!bestAp.dest.isValid() || ap.attackValue() > bestAp.attackValue())
			bestAp = ap;
	}

	// check how much damage we gain from blocking enemy shooters on this hex
	bestAp.shootersBlockedDmg = evaluateBlockedShootersDmg(attackInfo, hex, state);

#if BATTLE_TRACE_LEVEL>=1
	logAi->trace("BattleAI best AP: %s -> %s at %d from %d, affects %d units: d:%lld a:%lld c:%lld s:%lld",
		attackInfo.attacker->unitType()->getJsonKey(),
		attackInfo.defender->unitType()->getJsonKey(),
		(int)bestAp.dest, (int)bestAp.from, (int)bestAp.affectedUnits.size(),
		bestAp.defenderDamageReduce, bestAp.attackerDamageReduce, bestAp.collateralDamageReduce, bestAp.shootersBlockedDmg);
#endif

	//TODO other damage related to attack (eg. fire shield and other abilities)
	return bestAp;
}
