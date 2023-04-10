/*
 * OuterCaster.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "OuterCaster.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

OuterCaster::OuterCaster(const Caster * actualCaster_, int schoolLevel_)
	: ProxyCaster(actualCaster_), schoolLevel(schoolLevel_)
{

}

void OuterCaster::spendMana(ServerCallback * server, const int32_t spellCost) const
{
	//do nothing
}

int32_t OuterCaster::getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool) const
{
	return schoolLevel;
}

}

VCMI_LIB_NAMESPACE_END
