/*
 * Building.h, part of VCMI engine
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

#include "../../../lib/entities/building/CBuilding.h"

namespace scripting::api
{

class BuildingProxy : public RawPointerWrapper<const CBuilding, BuildingProxy>
{
public:
	static constexpr std::string_view luaName = "Building";
	static constexpr std::string_view luaDescription =
		"A building of a town, as `TownInstance:getBuildings` reports it.";

	static void registerMethods(MethodRegistrar & R);

	static std::string getJsonKey(const CBuilding & building);
	static std::optional<std::string> getBuildingType(const CBuilding & building);
	static bool isUpgrade(const CBuilding & building);
};

}
