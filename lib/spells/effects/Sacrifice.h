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
	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

protected:
	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;

private:
	static int64_t calculateHealEffectValue(const Mechanics * m, const battle::Unit * victim);
};

}
}

VCMI_LIB_NAMESPACE_END
