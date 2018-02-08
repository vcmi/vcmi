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

class AttackPossibility
{
public:
	BattleHex tile; //tile from which we attack
	BattleAttackInfo attack;

	std::shared_ptr<battle::CUnitState> attackerState;

	std::vector<std::shared_ptr<battle::CUnitState>> affectedUnits;

	int64_t damageDealt = 0;
	int64_t damageReceived = 0; //usually by counter-attack
	int64_t tacticImpact = 0;

	AttackPossibility(BattleHex tile_, const BattleAttackInfo & attack_);

	int64_t damageDiff() const;
	int64_t attackValue() const;

	static AttackPossibility evaluate(const BattleAttackInfo & attackInfo, BattleHex hex);
};
