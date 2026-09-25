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

	int chargeDistance = 0;
	bool shooting      = false;
	bool luckyStrike   = false;
	bool unluckyStrike = false;
	bool deathBlow     = false;
	bool doubleDamage  = false;
	/// Optional whole-stack base damage used by deterministic planners that
	/// evaluate the midpoint before applying combat modifiers.
	int64_t baseDamageOverride = -1;
	/// Optional exact Offense/Archery contribution. Classic battle AI uses this
	/// to preserve the fractional part of secondary-skill specialties.
	double offenseArcheryFactorOverride = -1.0;

	BattleAttackInfo(const battle::Unit * Attacker, const battle::Unit * Defender, int chargeDistance, bool Shooting);
	BattleAttackInfo reverse() const;
};
