/*
 * Enums.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Enums.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

Enums::EnumMap<EHealLevel> Enums::exportHealLevel() const
{
	return {
		{ "heal", EHealLevel::HEAL },
		{ "resurrect", EHealLevel::RESURRECT },
		{ "overheal", EHealLevel::OVERHEAL },
	};
}

Enums::EnumMap<EHealPower> Enums::exportHealPower() const
{
	return {
		{ "oneBattle", EHealPower::ONE_BATTLE },
		{ "permanent", EHealPower::PERMANENT },
	};
}

}

VCMI_LIB_NAMESPACE_END
