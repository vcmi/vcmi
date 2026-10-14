/*
 * Building.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Building.h"

#include "../../../lib/constants/StringConstants.h"

namespace scripting::api
{

void BuildingProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&BuildingProxy::getJsonKey>("getJsonKey",
		{"Identifier of this building, scoped by the mod providing it."},
		"Returns the json key of this building, such as `core:fort`.");
	R.function<&BuildingProxy::getBuildingType>("getBuildingType",
        {"'fort', 'villageHall', ...; nil for a building the game has no name of its own for."},
		"Returns which of the buildings known to the game this one is. Unlike the json key this is "
		"the same in every town, so it is what to test against when a rule speaks of a fort or a "
		"town hall rather than of one particular mod's version of it.");
	R.function<&BuildingProxy::isUpgrade>("isUpgrade",
		{"True when this building improves another one instead of standing on its own."},
		"Whether this building is an upgrade of another, as a citadel is of a fort.");
}

std::string BuildingProxy::getJsonKey(const CBuilding & building)
{
	return building.getJsonKey();
}

std::optional<std::string> BuildingProxy::getBuildingType(const CBuilding & building)
{
	// past the seventh upgraded dwelling the names stop lining up with the ids, and everything
	// beyond it is a mod's own building that the game has no name for anyway
	if(building.bid < BuildingID::FIRST_REGULAR_ID || building.bid > BuildingID::DWELL_LVL_7_UP)
		return std::nullopt;

	return EBuildingType::names[building.bid.getNum()];
}

bool BuildingProxy::isUpgrade(const CBuilding & building)
{
	return building.upgrade != BuildingID::NONE;
}

}
