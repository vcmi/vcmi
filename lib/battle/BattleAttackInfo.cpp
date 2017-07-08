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


BattleAttackInfo::BattleAttackInfo(const CStack * Attacker, const CStack * Defender, bool Shooting):
	attackerHealth(Attacker->health), defenderHealth(Defender->health)
{
	attacker = Attacker;
	defender = Defender;

	attackerBonuses = Attacker;
	defenderBonuses = Defender;

	attackerPosition = Attacker->position;
	defenderPosition = Defender->position;

	shooting = Shooting;
	chargedFields = 0;

	luckyHit = false;
	unluckyHit = false;
	deathBlow = false;
	ballistaDoubleDamage = false;
}

BattleAttackInfo BattleAttackInfo::reverse() const
{
	BattleAttackInfo ret = *this;
	std::swap(ret.attacker, ret.defender);
	std::swap(ret.attackerBonuses, ret.defenderBonuses);
	std::swap(ret.attackerPosition, ret.defenderPosition);
	std::swap(ret.attackerHealth, ret.defenderHealth);

	ret.shooting = false;
	ret.chargedFields = 0;
	ret.luckyHit = ret.ballistaDoubleDamage = ret.deathBlow = false;

	return ret;
}
