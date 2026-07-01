/*
 * SchoolService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../EntityService.h"

VCMI_LIB_NAMESPACE_BEGIN

class SpellSchool;

namespace spells
{

class SpellSchoolType;

class DLL_LINKAGE SchoolService : public EntityServiceT<SpellSchool, SpellSchoolType>
{
public:
	virtual std::vector<SpellSchool> getAllObjects() const = 0;
};

}

VCMI_LIB_NAMESPACE_END
