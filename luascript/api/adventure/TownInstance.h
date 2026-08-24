/*
 * TownInstance.h, part of VCMI engine
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

#include "../../../lib/mapObjects/CGTownInstance.h"

namespace scripting::api
{

class TownInstanceProxy : public RawPointerWrapper<const CGTownInstance, TownInstanceProxy>
{

public:
	static constexpr std::string_view luaName = "TownInstance";
	static constexpr std::string_view luaDescription =
		"A town on the adventure map. Provides its owner and what has been built in it.";

	static void registerMethods(MethodRegistrar & R);

	static std::vector<const CBuilding *> getBuildings(const CGTownInstance & town);
};

}
