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

VCMI_LIB_NAMESPACE_BEGIN

class CBattleInfoCallback;
class IBonusBearer;
class CSelector;
struct BattleAttackInfo;
struct DamageRange;

class DLL_LINKAGE DamageCalculator
{
	const CBattleInfoCallback & callback;
	const BattleAttackInfo & info;

	int battleBonusValue(const IBonusBearer * bearer, const CSelector & selector) const;

	DamageRange getBaseDamageSingle() const;
	DamageRange getBaseDamageBlessCurse() const;
	DamageRange getBaseDamageStack() const;

	int getActorAttackBase() const;
	int getActorAttackEffective() const;
	int getActorAttackSlayer() const;
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
	double getAttackHateFactor() const;

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

	std::vector<double> getAttackFactors() const;
	std::vector<double> getDefenseFactors() const;
public:
	DamageCalculator(const CBattleInfoCallback & callback, const BattleAttackInfo & info ):
		callback(callback),
		info(info)
	{}

	DamageRange calculateDmgRange() const;
};

VCMI_LIB_NAMESPACE_END
