/*
 * AbilityCaster.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AbilityCaster.h"

#include <vcmi/spells/Spell.h>

#include "../battle/Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

AbilityCaster::AbilityCaster(const battle::Unit * actualCaster_, int32_t baseSpellLevel_)
	: ProxyCaster(actualCaster_),
	actualCaster(actualCaster_),
	baseSpellLevel(baseSpellLevel_)
{
}

int32_t AbilityCaster::getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool) const
{
	auto skill = baseSpellLevel;

	if(spell->getLevel() > 0)
	{
		vstd::amax(skill, actualCaster->valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 0));
	}

	vstd::amax(skill, 0);
	vstd::amin(skill, 3);

	return static_cast<ui8>(skill); //todo: unify spell school level type
}

int32_t AbilityCaster::getEffectLevel(const Spell * spell) const
{
	return getSpellSchoolLevel(spell);
}

void AbilityCaster::getCastDescription(const Spell * spell, const std::vector<const battle::Unit*> & attacked, MetaString & text) const
{
	//do nothing
}

void AbilityCaster::spendMana(ServerCallback * server, const int32_t spellCost) const
{
	//do nothing
}

} // namespace spells

VCMI_LIB_NAMESPACE_END
