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

BattleAttackInfo::BattleAttackInfo(const battle::Unit * Attacker, const battle::Unit * Defender, bool Shooting)
	: attacker(Attacker),
	defender(Defender)
{
	shooting = Shooting;
	chargedFields = 0;
	additiveBonus = 0.0;
	multBonus = 1.0;
	attackerPos = BattleHex::INVALID;
	defenderPos = BattleHex::INVALID;
}

BattleAttackInfo BattleAttackInfo::reverse() const
{
	BattleAttackInfo ret = *this;

	std::swap(ret.attacker, ret.defender);
	std::swap(ret.defenderPos, ret.attackerPos);

	ret.shooting = false;
	ret.chargedFields = 0;

	ret.additiveBonus = 0.0;
	ret.multBonus = 1.0;

	return ret;
}

VCMI_LIB_NAMESPACE_END
