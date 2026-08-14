/*
 * MapScriptInit.h, part of VCMI engine
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

#include <vcmi/scripting/ApiTags.h>

class CMap;
class CGObjectInstance;

namespace scripting::api
{

/// Transient handle passed to a map script's `init` method so it can enumerate the map's objects and
/// bind its handler functions to event objects by instance name. Valid only for the duration of init.
struct MapScriptInit : public scripting::ApiRawPointer<MapScriptInit>
{
	CMap & map;
	explicit MapScriptInit(CMap & map): map(map) {}
};

class MapScriptInitProxy : public RawPointerWrapper<MapScriptInit, MapScriptInitProxy>
{
	static void attachEventScript(MapScriptInit & object, const std::string & funcName, const std::string & objectName);
	static std::vector<const CGObjectInstance *> objects(const MapScriptInit & object);

public:
	static constexpr std::string_view luaName = "MapSetup";
	static constexpr std::string_view luaDescription =
		"Setup handle given to a map script's init method. Lets the script list the map's objects and "
		"bind its handler functions to event objects by their instance name.";

	static void registerMethods(MethodRegistrar & R);
};

}
