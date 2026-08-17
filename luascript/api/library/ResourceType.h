/*
 * api/ResourceType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/ResourceType.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

namespace scripting::api
{

class ResourceTypeProxy : public RawPointerWrapper<const ResourceType, ResourceTypeProxy>
{
public:
	static constexpr std::string_view luaName = "ResourceType";
	static constexpr std::string_view luaDescription =
		"A resource definition (wood, ore, gold, ...). Obtained from Services:getResourceByName and "
		"passed to the calls that add or read a player's resources.";

	static void registerMethods(MethodRegistrar & R);
};

}
