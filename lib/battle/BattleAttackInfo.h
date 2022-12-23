/*
 * BattleAttackInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{
	class Unit;
	class CUnitState;
}

struct DLL_LINKAGE BattleAttackInfo
{
	const battle::Unit * attacker;
	const battle::Unit * defender;

	BattleHex attackerPos;
	BattleHex defenderPos;

	bool shooting;
	int chargedFields;

	double additiveBonus;
	double multBonus;

	BattleAttackInfo(const battle::Unit * Attacker, const battle::Unit * Defender, bool Shooting = false);
	BattleAttackInfo reverse() const;
};

VCMI_LIB_NAMESPACE_END
