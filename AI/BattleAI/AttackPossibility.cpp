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

AttackPossibility::AttackPossibility(BattleHex from, BattleHex dest, const BattleAttackInfo & attack)
	: from(from), dest(dest), attack(attack)
{
}

int64_t AttackPossibility::damageDiff() const
{
	return damageDealt - damageReceived - collateralDamage + shootersBlockedDmg;
}

int64_t AttackPossibility::attackValue() const
{
	return damageDiff();
}

int64_t AttackPossibility::evaluateBlockedShootersDmg(const BattleAttackInfo & attackInfo, BattleHex hex, const HypotheticBattle * state)
{
	int64_t res = 0;

	if(attackInfo.shooting)
		return 0;

	auto attacker = attackInfo.attacker;
	auto hexes = attacker->getSurroundingHexes(hex);
	for(BattleHex tile : hexes)
	{
		auto st = state->battleGetUnitByPos(tile, true);
		if(!st || !state->battleMatchOwner(st, attacker))
			continue;
		if(!state->battleCanShoot(st))
			continue;

		BattleAttackInfo rangeAttackInfo(st, attacker, true);
		rangeAttackInfo.defenderPos = hex;

		BattleAttackInfo meleeAttackInfo(st, attacker, false);
		meleeAttackInfo.defenderPos = hex;

		auto rangeDmg = getCbc()->battleEstimateDamage(rangeAttackInfo);
		auto meleeDmg = getCbc()->battleEstimateDamage(meleeAttackInfo);

		int64_t gain = (rangeDmg.first + rangeDmg.second - meleeDmg.first - meleeDmg.second) / 2 + 1;
		res += gain;
	}

	return res;
}

AttackPossibility AttackPossibility::evaluate(const BattleAttackInfo & attackInfo, BattleHex hex, const HypotheticBattle * state)
{
	auto attacker = attackInfo.attacker;
	auto defender = attackInfo.defender;
	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type()(Bonus::BLOCKS_RETALIATION);
	const auto attackerSide = getCbc()->playerToSide(getCbc()->battleGetOwner(attacker));
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
			units = state->getAttackedBattleUnits(attacker, defHex, true, BattleHex::INVALID);
		else
			units = state->getAttackedBattleUnits(attacker, defHex, false, hex);

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
				int64_t damageDealt, damageReceived, enemyDpsReduce, ourDpsReduce;

				TDmgRange retaliation(0, 0);
				auto attackDmg = getCbc()->battleEstimateDamage(ap.attack, &retaliation);
				TDmgRange enemyDamageBeforeAttack = getCbc()->battleEstimateDamage(BattleAttackInfo(u, attacker, u->canShoot()));

				vstd::amin(attackDmg.first, defenderState->getAvailableHealth());
				vstd::amin(attackDmg.second, defenderState->getAvailableHealth());

				vstd::amin(retaliation.first, ap.attackerState->getAvailableHealth());
				vstd::amin(retaliation.second, ap.attackerState->getAvailableHealth());

				damageDealt = (attackDmg.first + attackDmg.second) / 2;
				ap.attackerState->afterAttack(attackInfo.shooting, false);

				auto enemiesKilled = damageDealt / u->MaxHealth() + (damageDealt % u->MaxHealth() >= u->getFirstHPleft() ? 1 : 0);
				auto enemyDps = (enemyDamageBeforeAttack.first + enemyDamageBeforeAttack.second) / 2;

				enemyDpsReduce = enemiesKilled
					? (int64_t)(enemyDps * enemiesKilled / (double)u->getCount())
					: (int64_t)(enemyDps / (double)u->getCount() * damageDealt / u->getFirstHPleft());

				//FIXME: use ranged retaliation
				damageReceived = 0;
				ourDpsReduce = 0;

				if (!attackInfo.shooting && defenderState->ableToRetaliate() && !counterAttacksBlocked)
				{
					damageReceived = (retaliation.first + retaliation.second) / 2;
					defenderState->afterAttack(attackInfo.shooting, true);

					auto ourUnitsKilled = damageReceived / attacker->MaxHealth() + (damageReceived % attacker->MaxHealth() >= attacker->getFirstHPleft() ? 1 : 0);

					ourDpsReduce = (int64_t)(damageDealt * ourUnitsKilled / (double)attacker->getCount());
				}

				bool isEnemy = state->battleMatchOwner(attacker, u);

				// this includes enemy units as well as attacker units under enemy's mind control
				if(isEnemy)
					ap.damageDealt += enemyDpsReduce;

				// damaging attacker's units (even those under enemy's mind control) is considered friendly fire
				if(attackerSide == u->unitSide())
					ap.collateralDamage += enemyDpsReduce;

				if(u->unitId() == defender->unitId() || 
					(!attackInfo.shooting && CStack::isMeleeAttackPossible(u, attacker, hex)))
				{
					//FIXME: handle RANGED_RETALIATION ?
					ap.damageReceived += ourDpsReduce;
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

	logAi->debug("BattleAI best AP: %s -> %s at %d from %d, affects %d units: %lld %lld %lld %lld",
		attackInfo.attacker->unitType()->identifier,
		attackInfo.defender->unitType()->identifier,
		(int)bestAp.dest, (int)bestAp.from, (int)bestAp.affectedUnits.size(),
		bestAp.damageDealt, bestAp.damageReceived, bestAp.collateralDamage, bestAp.shootersBlockedDmg);

	//TODO other damage related to attack (eg. fire shield and other abilities)
	return bestAp;
}
