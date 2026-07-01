/*
 * api/SpellSchool.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../../lib/spells/SpellSchoolHandler.h"

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class SpellSchoolProxy : public RawPointerWrapper<const spells::SpellSchoolType, SpellSchoolProxy>
{
public:
	static constexpr std::string_view luaName = "SpellSchool";
	static constexpr std::string_view luaDescription =
		"A spell school (Fire, Water, Earth, Air, …). Identifies which school a spell belongs to "
		"and is used for school-based immunity and reduction checks.";

	static void registerMethods(MethodRegistrar & R);
};

}

VCMI_LIB_NAMESPACE_END
