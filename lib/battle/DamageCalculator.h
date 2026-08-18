/*
 * DamageCalculator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

class CBattleInfoCallback;
class IBonusBearer;
class CSelector;
struct BattleAttackInfo;
struct DamageRange;
struct DamageEstimation;

namespace battle
{
class Unit;
}

class DLL_LINKAGE DamageCalculator
{
	const CBattleInfoCallback & callback;
	const BattleAttackInfo & info;

	int battleBonusValue(const IBonusBearer * bearer, const CSelector & selector) const;

	DamageRange getCasualties(const DamageRange & damageDealt) const;
	int64_t getCasualties(int64_t damageDealt) const;

	DamageRange getBaseDamageSingle() const;
	DamageRange getBaseDamageBlessCurse() const;
	DamageRange getBaseDamageStack() const;

	/// Amount of defense that the given unit makes its opponent ignore, as a negative number.
	int getDefenseIgnored(const battle::Unit * reducer, int defense) const;

	int getActorAttackBase() const;
	int getActorAttackEffective() const;
	int getActorAttackFrenzy() const;
	int getActorAttackSlayer() const;
	int getActorAttackIgnored() const;
	int getTargetDefenseBase() const;
	int getTargetDefenseEffective() const;
	int getTargetDefenseIgnored() const;

	double getAttackSkillFactor() const;
	double getAttackOffenseArcheryFactor() const;
	double getAttackBlessFactor() const;
	double getAttackLuckFactor() const;
	double getAttackJoustingFactor() const;
	double getAttackDeathBlowFactor() const;
	double getAttackDoubleDamageFactor() const;
	double getAttackHateCreatureFactor() const;
	double getAttackHateTraitFactor() const;
	double getAttackRevengeFactor() const;
	double getAttackFromBackFactor() const;

	double getDefenseSkillFactor() const;
	double getDefenseArmorerFactor() const;
	double getDefenseMagicShieldFactor() const;
	double getDefenseRangePenaltiesFactor() const;
	double getDefenseObstacleFactor() const;
	double getDefenseBlindParalysisFactor() const;
	double getDefenseUnluckyFactor() const;
	double getDefenseForgetfulnessFactor() const;
	double getDefensePetrificationFactor() const;
	double getDefenseMagicFactor() const;
	double getDefenseMindFactor() const;
	int64_t getDamageCap() const;

	/// Every factor of the attack, signed - positive raises the damage, negative lowers it.
	std::vector<double> getDamageFactors() const;
public:
	DamageCalculator(const CBattleInfoCallback & callback, const BattleAttackInfo & info ):
		callback(callback),
		info(info)
	{}

	DamageEstimation calculateDmgRange() const;
};
