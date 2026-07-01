/*
 * api/HeroType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/HeroType.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{
class HeroTypeProxy : public RawPointerWrapper<const HeroType, HeroTypeProxy>
{
public:
	static constexpr std::string_view luaName = "HeroType";
	static constexpr std::string_view luaDescription =
		"A specific named hero from the database (Sir Mullich, Crag Hack, …). Identifies the "
		"hero kind and their starting profile; the in-game placed hero is HeroInstance.";

	static void registerMethods(MethodRegistrar & R);
};

}

VCMI_LIB_NAMESPACE_END
