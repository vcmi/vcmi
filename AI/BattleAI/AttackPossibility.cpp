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
#include "../../lib/CStack.h"//todo: remove

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
	static const auto selectorBlocksRetaliation = Selector::type(Bonus::BLOCKS_RETALIATION);
	const bool counterAttacksBlocked = attacker->hasBonus(selectorBlocksRetaliation, cachingStringBlocksRetaliation);
	const bool mindControlled = [&](const battle::Unit *attacker) -> bool
	{
		auto actualSide = getCbc()->playerToSide(getCbc()->battleGetOwner(attacker));
		if (actualSide && actualSide.get() != attacker->unitSide())
			return true;
		return false;
	} (attacker);

	AttackPossibility bestAp(hex, BattleHex::INVALID, attackInfo);

	std::vector<BattleHex> defenderHex;
	if(attackInfo.shooting)
		defenderHex = defender->getHexes();
	else
		defenderHex = CStack::meleeAttackHexes(attacker, defender, hex);

	for(BattleHex defHex : defenderHex)
	{
		if(defHex == hex) {
			// should be impossible but check anyway
			continue;
		}

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
		for(auto unit : units) {
			if (unit->unitId() == defender->unitId()) {
				addDefender = false;
				break;
			}
		}
		if(addDefender) {
			units.push_back(defender);
		}

		for(auto u : units) {
			if(!ap.attackerState->alive()) {
				break;
			}

			assert(u->unitId() != attacker->unitId());

			auto defenderState = u->acquireState();
			ap.affectedUnits.push_back(defenderState);

			for(int i = 0; i < totalAttacks; i++) {
				si64 damageDealt, damageReceived;

				TDmgRange retaliation(0, 0);
				auto attackDmg = getCbc()->battleEstimateDamage(ap.attack, &retaliation);

				vstd::amin(attackDmg.first, defenderState->getAvailableHealth());
				vstd::amin(attackDmg.second, defenderState->getAvailableHealth());

				vstd::amin(retaliation.first, ap.attackerState->getAvailableHealth());
				vstd::amin(retaliation.second, ap.attackerState->getAvailableHealth());

				damageDealt = (attackDmg.first + attackDmg.second) / 2;
				ap.attackerState->afterAttack(attackInfo.shooting, false);

				//FIXME: use ranged retaliation
				damageReceived = 0;
				if (!attackInfo.shooting && defenderState->ableToRetaliate() && !counterAttacksBlocked)
				{
					damageReceived = (retaliation.first + retaliation.second) / 2;
					defenderState->afterAttack(attackInfo.shooting, true);
				}

				bool isEnemy = state->battleMatchOwner(attacker, u) && !mindControlled;
				if(isEnemy)
					ap.damageDealt += damageDealt;
				else // friendly fire
					ap.collateralDamage += damageDealt;

				if(u->unitId() == defender->unitId() || 
					(!attackInfo.shooting && CStack::isMeleeAttackPossible(u, attacker, hex)))
				{
					//FIXME: handle RANGED_RETALIATION ?
					ap.damageReceived += damageReceived;
				}

				ap.attackerState->damage(damageReceived);
				defenderState->damage(damageDealt);

				if (!ap.attackerState->alive() || !defenderState->alive())
					break;
			}
		}

		if(!bestAp.dest.isValid() || ap.attackValue() > bestAp.attackValue()) {
			bestAp = ap;
		}
	}

	// check how much damage we gain from blocking enemy shooters on this hex
	bestAp.shootersBlockedDmg = evaluateBlockedShootersDmg(attackInfo, hex, state);

	logAi->debug("BattleAI best AP: %s -> %s at %d from %d, affects %d units: %d %d %d %s",
		VLC->creh->creatures.at(attackInfo.attacker->acquireState()->creatureId())->identifier.c_str(),
		VLC->creh->creatures.at(attackInfo.defender->acquireState()->creatureId())->identifier.c_str(),
		(int)bestAp.dest, (int)bestAp.from, (int)bestAp.affectedUnits.size(),
		(int)bestAp.damageDealt, (int)bestAp.damageReceived, (int)bestAp.collateralDamage, (int)bestAp.shootersBlockedDmg);

	//TODO other damage related to attack (eg. fire shield and other abilities)
	return bestAp;
}
