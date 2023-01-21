/*
 * BattleAttackInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleAttackInfo.h"
#include "CUnitState.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleAttackInfo::BattleAttackInfo(const battle::Unit * Attacker, const battle::Unit * Defender, int chargeDistance, bool Shooting)
	: attacker(Attacker),
	defender(Defender),
	shooting(Shooting),
	attackerPos(BattleHex::INVALID),
	defenderPos(BattleHex::INVALID),
	chargeDistance(chargeDistance)
{}

BattleAttackInfo BattleAttackInfo::reverse() const
{
	BattleAttackInfo ret(defender, attacker, 0, false);

	ret.defenderPos = attackerPos;
	ret.attackerPos = defenderPos;
	return ret;
}

VCMI_LIB_NAMESPACE_END
