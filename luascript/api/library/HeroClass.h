/*
 * api/HeroClass.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/HeroClass.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class HeroClassProxy : public RawPointerWrapper<const HeroClass, HeroClassProxy>
{
public:
	static constexpr std::string_view luaName = "HeroClass";
	static constexpr std::string_view luaDescription =
		"Hero class definition (Knight, Cleric, Ranger, …). Static metadata shared by all "
		"heroes of that class; for a placed hero use HeroInstance.";

	static void registerMethods(MethodRegistrar & R);
};

}

VCMI_LIB_NAMESPACE_END
