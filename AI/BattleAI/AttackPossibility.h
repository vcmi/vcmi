/*
 * AttackPossibility.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../../lib/battle/CUnitState.h"
#include "../../CCallback.h"
#include "common.h"
#include "StackWithBonuses.h"

#define BATTLE_TRACE_LEVEL 0

class DamageCache
{
private:
	std::unordered_map<uint32_t, std::unordered_map<uint32_t, float>> damageCache;
	DamageCache * parent;

public:
	DamageCache() : parent(nullptr) {}
	DamageCache(DamageCache * parent) : parent(parent) {}

	void cacheDamage(const battle::Unit * attacker, const battle::Unit * defender, std::shared_ptr<CBattleInfoCallback> hb);
	int64_t getDamage(const battle::Unit * attacker, const battle::Unit * defender, std::shared_ptr<CBattleInfoCallback> hb);
	int64_t getOriginalDamage(const battle::Unit * attacker, const battle::Unit * defender, std::shared_ptr<CBattleInfoCallback> hb);
	void buildDamageCache(std::shared_ptr<HypotheticBattle> hb, int side);
};

/// <summary>
/// Evaluate attack value of one particular attack taking into account various effects like
/// retaliation, 2-hex breath, collateral damage, shooters blocked damage
/// </summary>
class AttackPossibility
{
public:
	BattleHex from; //tile from which we attack
	BattleHex dest; //tile which we attack
	BattleAttackInfo attack;

	std::shared_ptr<battle::CUnitState> attackerState;

	std::vector<std::shared_ptr<battle::CUnitState>> affectedUnits;

	int64_t defenderDamageReduce = 0;
	int64_t attackerDamageReduce = 0; //usually by counter-attack
	int64_t collateralDamageReduce = 0; // friendly fire (usually by two-hex attacks)
	int64_t shootersBlockedDmg = 0;

	AttackPossibility(BattleHex from, BattleHex dest, const BattleAttackInfo & attack_);

	int64_t damageDiff() const;
	int64_t attackValue() const;

	static AttackPossibility evaluate(
		const BattleAttackInfo & attackInfo,
		BattleHex hex,
		DamageCache & damageCache,
		std::shared_ptr<CBattleInfoCallback> state);

	static int64_t calculateDamageReduce(
		const battle::Unit * attacker,
		const battle::Unit * defender,
		uint64_t damageDealt,
		DamageCache & damageCache,
		std::shared_ptr<CBattleInfoCallback> cb);

private:
	static int64_t evaluateBlockedShootersDmg(
		const BattleAttackInfo & attackInfo,
		BattleHex hex,
		DamageCache & damageCache,
		std::shared_ptr<CBattleInfoCallback> state);
};
