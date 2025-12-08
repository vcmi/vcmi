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
#include "lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

struct SpellEffectValue
{
	int64_t hpDelta = 0; // positive -> healed health points, negative -> damage
	int64_t unitsDelta = 0; // positive -> resurrected / summoned (demons) / animated (undeads), negative -> kills
	CreatureID unitType = CreatureID::NONE; // type of creatures summoned / resurrected / animated / etc.

	SpellEffectValue & operator+=(const SpellEffectValue & rhs) noexcept
	{
		hpDelta += rhs.hpDelta;
		unitsDelta += rhs.unitsDelta;
		if(unitType == CreatureID::NONE)
			unitType = rhs.unitType;

		return *this;
	}
};

class UnitEffect : public Effect
{
public:
	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	void adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget filterTarget(const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

    bool getStackFilter(const Mechanics * m, bool alwaysSmart, const battle::Unit * s) const;

    virtual bool eraseByImmunityFilter(const Mechanics * m, const battle::Unit * s) const;

	virtual SpellEffectValue getHealthChange(const Mechanics * m, const EffectTarget & spellTarget) const;

protected:
	int32_t chainLength = 0;
	double chainFactor = 0.0;

	virtual bool isReceptive(const Mechanics * m, const battle::Unit * unit) const;
	virtual bool isSmartTarget(const Mechanics * m, const battle::Unit * unit, bool alwaysSmart) const;
	virtual bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const;

	void serializeJsonEffect(JsonSerializeFormat & handler) override final;
	virtual void serializeJsonUnitEffect(JsonSerializeFormat & handler) = 0;

private:
	bool ignoreImmunity = false;

	EffectTarget transformTargetByRange(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;
	EffectTarget transformTargetByChain(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;
};

}
}

VCMI_LIB_NAMESPACE_END
