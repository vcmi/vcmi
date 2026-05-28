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

class DLL_LINKAGE UnitEffect : public Effect
{
public:
	void adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const final;

	bool applicableGeneral(Problem & problem, const Mechanics * m) const override;
	bool applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const override;
	bool applicableUnit(const Mechanics * m, const battle::Unit * s, bool alwaysSmart, bool alwaysReceptive) const;

	Target filterTarget(const Mechanics * m, const Target & target) const final;
	Target transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	int32_t chainLength = 0;
	double chainFactor = 0.0;

	virtual bool isReceptive(const Mechanics * m, const battle::Unit * unit) const;
	virtual bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const;

	void serializeJsonEffect(JsonSerializeFormat & handler) final;
	virtual void serializeJsonUnitEffect(JsonSerializeFormat & handler) = 0;

private:
	bool ignoreImmunity = false;

	Target transformTargetByRange(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;
	Target transformTargetByChain(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const;
};

}
}

VCMI_LIB_NAMESPACE_END
