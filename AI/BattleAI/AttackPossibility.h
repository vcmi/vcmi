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

class AttackPossibility
{
public:
	BattleHex from; //tile from which we attack
	BattleHex dest; //tile which we attack
	BattleAttackInfo attack;

	std::shared_ptr<battle::CUnitState> attackerState;

	std::vector<std::shared_ptr<battle::CUnitState>> affectedUnits;

	int64_t damageDealt = 0;
	int64_t damageReceived = 0; //usually by counter-attack
	int64_t collateralDamage = 0; // friendly fire (usually by two-hex attacks)
	int64_t shootersBlockedDmg = 0;

	AttackPossibility(BattleHex from, BattleHex dest, const BattleAttackInfo & attack_);

	int64_t damageDiff() const;
	int64_t attackValue() const;

	static AttackPossibility evaluate(const BattleAttackInfo & attackInfo, BattleHex hex, const HypotheticBattle * state);

	static int64_t calculateDpsReduce(
		const battle::Unit * attacker,
		const battle::Unit * defender,
		uint64_t damageDealt,
		std::shared_ptr<CBattleInfoCallback> cb);

private:
	static int64_t evaluateBlockedShootersDmg(const BattleAttackInfo & attackInfo, BattleHex hex, const HypotheticBattle * state);
};
