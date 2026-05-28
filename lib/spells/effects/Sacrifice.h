/*
 * Sacrifice.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Heal.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class Sacrifice : public Heal
{
public:
	void adjustTargetTypes(std::vector<TargetType> & types, const Mechanics * m) const override;

	bool applicableGeneral(Problem & problem, const Mechanics * m) const override;
	bool applicableTarget(Problem & problem, const Mechanics * m, const Target & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const Target & target) const override;

	Target transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

	SpellEffectValue getHealthChange(const Mechanics * m, const Target & spellTarget) const final;

protected:
	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;

private:
	static int64_t calculateHealEffectValue(const Mechanics * m, const battle::Unit * victim);
};

}
}

VCMI_LIB_NAMESPACE_END
