/*
 * LocationEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LocationEffect.h"
#include "../ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

void LocationEffect::adjustTargetTypes(std::vector<TargetType> & types) const
{

}

void LocationEffect::adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const
{
	for(const auto & destnation : spellTarget)
		hexes.insert(destnation.hexValue);
}

EffectTarget LocationEffect::filterTarget(const Mechanics * m, const EffectTarget & target) const
{
	EffectTarget res;
	vstd::copy_if(target, std::back_inserter(res), [](const Destination & d)
	{
		return !d.unitValue && (d.hexValue.isValid());
	});
	return res;
}

EffectTarget LocationEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//by default effect covers exactly spell range
	return EffectTarget(spellTarget);
}

}
}

VCMI_LIB_NAMESPACE_END
