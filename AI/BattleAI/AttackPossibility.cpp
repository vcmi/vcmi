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

void DamageCache::cacheDamage(const battle::Unit * attacker, const battle::Unit * defender, std::shared_ptr<CBattleInfoCallback> hb)
{
	auto damage = averageDmg(hb->battleEstimateDamage(attacker, defender, 0).damage);

	damageCache[attacker->unitId()][defender->unitId()] = static_cast<float>(damage) / attacker->getCount();
}


void DamageCache::buildDamageCache(std::shared_ptr<HypotheticBattle> hb, int side)
{
	auto stacks = hb->battleGetUnitsIf([=](const battle::Unit * u) -> bool
		{
			return u->isValidTarget();
		});

	std::vector<const battle::Unit *> ourUnits, enemyUnits;

	for(auto stack : stacks)
	{
		if(stack->unitSide() == side)
			ourUnits.push_back(stack);
		else
			enemyUnits.push_back(stack);
	}

	for(auto ourUnit : ourUnits)
	{
		if(!ourUnit->alive())
			continue;

		for(auto enemyUnit : enemyUnits)
		{
			if(enemyUnit->alive())
			{
				cacheDamage(ourUnit, enemyUnit, hb);
				cacheDamage(enemyUnit, ourUnit, hb);
			}
		}
	}
}

int64_t DamageCache::getDamage(const battle::Unit * attacker, const battle::Unit * defender, std::shared_ptr<CBattleInfoCallback> hb)
{
	auto damage = damageCache[attacker->unitId()][defender->unitId()] * attacker->getCount();

	if(damage == 0)
	{
		cacheDamage(attacker, defender, hb);

		damage = damageCache[attacker->unitId()][defender->unitId()] * attacker->getCount();
	}

	return static_cast<int64_t>(damage);
}

int64_t DamageCache::getOriginalDamage(const battle::Unit * attacker, const battle::Unit * defender, std::shared_ptr<CBattleInfoCallback> hb)
{
	if(parent)
	{
		auto attackerDamageMap = parent->damageCache.find(attacker->unitId());

		if(attackerDamageMap != parent->damageCache.end())
		{
			auto targetDamage = attackerDamageMap->second.find(defender->unitId());

			if(targetDamage != attackerDamageMap->second.end())
			{
				return static_cast<int64_t>(targetDamage->second * attacker->getCount());
			}
		}
	}

	return getDamage(attacker, defender, hb);
}

AttackPossibility::AttackPossibility(BattleHex from, BattleHex dest, const BattleAttackInfo & attack)
	: from(from), dest(dest), attack(attack)
{
}

float AttackPossibility::damageDiff() const
{
	return defenderDamageReduce - attackerDamageReduce - collateralDamageReduce + shootersBlockedDmg;
}

float AttackPossibility::damageDiff(float positiveEffectMultiplier, float negativeEffectMultiplier) const
{
	return positiveEffectMultiplier * (defenderDamageReduce + shootersBlockedDmg)
		- negativeEffectMultiplier * (attackerDamageReduce + collateralDamageReduce);
}

float AttackPossibility::attackValue() const
{
	return damageDiff();
}

/// <summary>
/// How enemy damage will be reduced by this attack
/// Half bounty for kill, half for making damage equal to enemy health
/// Bounty - the killed creature average damage calculated against attacker
/// </summary>
float AttackPossibility::calculateDamageReduce(
	const battle::Unit * attacker,
	const battle::Unit * defender,
	uint64_t damageDealt,
	DamageCache & damageCache,
	std::shared_ptr<CBattleInfoCallback> state)
{
	const float HEALTH_BOUNTY = 0.5;

	// FIXME: provide distance info for Jousting bonus
	auto attackerUnitForMeasurement = attacker;

	if(!attackerUnitForMeasurement || attackerUnitForMeasurement->isTurret())
	{
		auto ourUnits = state->battleGetUnitsIf([&](const battle::Unit * u) -> bool
			{
				return u->unitSide() != defender->unitSide()
					&& !u->isTurret()
					&& u->creatureId() != CreatureID::CATAPULT
					&& u->creatureId() != CreatureID::BALLISTA
					&& u->creatureId() != CreatureID::FIRST_AID_TENT
					&& u->getCount();
			});

		if(ourUnits.empty())
			attackerUnitForMeasurement = defender;
		else
			attackerUnitForMeasurement = ourUnits.front();
	}

	auto maxHealth = defender->getMaxHealth();
	auto availableHealth = defender->getFirstHPleft() + ((defender->getCount() - 1) * maxHealth);

	vstd::amin(damageDealt, availableHealth);

	auto enemyDamageBeforeAttack = damageCache.getOriginalDamage(defender, attackerUnitForMeasurement, state);
	auto enemiesKilled = damageDealt / maxHealth + (damageDealt % maxHealth >= defender->getFirstHPleft() ? 1 : 0);
	auto damagePerEnemy = enemyDamageBeforeAttack / (double)defender->getCount();
	
	// lets use cached maxHealth here instead of getAvailableHealth
	auto firstUnitHpLeft = (availableHealth - damageDealt) % maxHealth;
	auto firstUnitHealthRatio = firstUnitHpLeft == 0 ? 1 : static_cast<float>(firstUnitHpLeft) / maxHealth;
	auto firstUnitKillValue = (1 - firstUnitHealthRatio) * (1 - firstUnitHealthRatio);

	return damagePerEnemy * (enemiesKilled + firstUnitKillValue * HEALTH_BOUNTY);
}

int64_t AttackPossibility::evaluateBlockedShootersDmg(
	const BattleAttackInfo & attackInfo,
	BattleHex hex,
	DamageCache & damageCache,
	std::shared_ptr<CBattleInfoCallback> state)
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

		// FIXME: provide distance info for Jousting bonus
		BattleAttackInfo rangeAttackInfo(st, attacker, 0, true);
		rangeAttackInfo.defenderPos = hex;

		BattleAttackInfo meleeAttackInfo(st, attacker, 0, false);
		meleeAttackInfo.defenderPos = hex;

		auto rangeDmg = state->battleEstimateDamage(rangeAttackInfo);
		auto meleeDmg = state->battleEstimateDamage(meleeAttackInfo);

		int64_t gain = averageDmg(rangeDmg.damage) - averageDmg(meleeDmg.damage) + 1;
		res += gain;
	}

	return res;
}

AttackPossibility AttackPossibility::evaluate(
	const BattleAttackInfo & attackInfo,
	BattleHex hex,
	DamageCache & damageCache,
	std::shared_ptr<CBattleInfoCallback> state)
{
	auto attacker = attackInfo.attacker;
	auto defender = attackInfo.defender;
	const std::string cachingStringBlocksRetaliation = "type_BLOCKS_RETALIATION";
	static const auto selectorBlocksRetaliation = Selector::type()(BonusType::BLOCKS_RETALIATION);
	const auto attackerSide = state->playerToSide(state->battleGetOwner(attacker));
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
				int64_t damageDealt, damageReceived;
				float defenderDamageReduce, attackerDamageReduce;

				DamageEstimation retaliation;
				auto attackDmg = state->battleEstimateDamage(ap.attack, &retaliation);

				vstd::amin(attackDmg.damage.min, defenderState->getAvailableHealth());
				vstd::amin(attackDmg.damage.max, defenderState->getAvailableHealth());

				vstd::amin(retaliation.damage.min, ap.attackerState->getAvailableHealth());
				vstd::amin(retaliation.damage.max, ap.attackerState->getAvailableHealth());

				damageDealt = averageDmg(attackDmg.damage);
				defenderDamageReduce = calculateDamageReduce(attacker, defender, damageDealt, damageCache, state);
				ap.attackerState->afterAttack(attackInfo.shooting, false);

				//FIXME: use ranged retaliation
				damageReceived = 0;
				attackerDamageReduce = 0;

				if (!attackInfo.shooting && defenderState->ableToRetaliate() && !counterAttacksBlocked)
				{
					damageReceived = averageDmg(retaliation.damage);
					attackerDamageReduce = calculateDamageReduce(defender, attacker, damageReceived, damageCache, state);
					defenderState->afterAttack(attackInfo.shooting, true);
				}

				bool isEnemy = state->battleMatchOwner(attacker, u);

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
	bestAp.shootersBlockedDmg = evaluateBlockedShootersDmg(attackInfo, hex, damageCache, state);

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
