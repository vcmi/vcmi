/*
 * UnitEffect.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class UnitEffect : public Effect
{
public:
	UnitEffect();
	virtual ~UnitEffect();

	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	void adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

    bool getStackFilter(const Mechanics * m, bool alwaysSmart, const battle::Unit * s) const;

    virtual bool eraseByImmunityFilter(const Mechanics * m, const battle::Unit * s) const;

protected:
	int32_t chainLength;
	double chainFactor;

	virtual bool isReceptive(const Mechanics * m, const battle::Unit * unit) const;
	virtual bool isSmartTarget(const Mechanics * m, const battle::Unit * unit, bool alwaysSmart) const;
	virtual bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const;

	void serializeJsonEffect(JsonSerializeFormat & handler) override final;
	virtual void serializeJsonUnitEffect(JsonSerializeFormat & handler) = 0;

private:
	bool ignoreImmunity;

	EffectTarget transformTargetByRange(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;
	EffectTarget transformTargetByChain(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;
};

}
}

VCMI_LIB_NAMESPACE_END
