/*
 * MapObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

#include "../../../lib/mapObjects/CGObjectInstance.h"

namespace scripting::api
{

class MapObjectProxy : public RawPointerWrapper<const CGObjectInstance, MapObjectProxy>
{
public:
	static constexpr std::string_view luaName = "MapObject";
	static constexpr std::string_view luaDescription =
		"A handle to an adventure-map object (event, town, monster, resource, ...). Right now used only to "
		"identify the object a trigger fired on, so scripts can act on it (e.g. remove it).";

	static void registerMethods(MethodRegistrar & R);

	static std::string getInstanceName(const CGObjectInstance & object);
};

}
