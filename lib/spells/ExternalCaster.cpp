/*
 * ExternalCaster.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ExternalCaster.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{

ExternalCaster::ExternalCaster()
	: ProxyCaster(nullptr), schoolLevel(0)
{
}

ExternalCaster::ExternalCaster(const Caster * actualCaster_, int schoolLevel_)
	: ProxyCaster(actualCaster_), schoolLevel(schoolLevel_)
{
}

void ExternalCaster::setActualCaster(const Caster * actualCaster_)
{
	actualCaster = actualCaster_;
}

void ExternalCaster::setSpellSchoolLevel(int level)
{
	schoolLevel = level;
}

void ExternalCaster::spendMana(ServerCallback * server, const int32_t spellCost) const
{
	//do nothing
}

int32_t ExternalCaster::getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool) const
{
	return schoolLevel;
}

}

VCMI_LIB_NAMESPACE_END
