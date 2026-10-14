/*
 * TownInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "TownInstance.h"

#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/mapObjects/CGObjectInstance.h"

namespace scripting::api
{

void TownInstanceProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&CGObjectInstance::getOwner, CGTownInstance>("getOwner", {},
		"Returns the player color that owns this town, or the neutral player when it is unowned.");
	R.function<&TownInstanceProxy::getBuildings>("getBuildings",
		{"Every building standing in this town."},
		"Returns the buildings that have been built in this town, upgrades of other buildings among them.");
}

std::vector<const CBuilding *> TownInstanceProxy::getBuildings(const CGTownInstance & town)
{
	std::vector<const CBuilding *> result;

	for(const auto & buildingID : town.getBuildings())
		result.push_back(town.getTown()->buildings.at(buildingID).get());

	return result;
}

}
