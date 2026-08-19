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

namespace scripting::api
{

void BuildingProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&BuildingProxy::getJsonKey>("getJsonKey",
		{"Identifier of this building, scoped by the mod providing it."},
		"Returns the json key of this building, such as `core:fort`.");
	R.function<&BuildingProxy::isUpgrade>("isUpgrade",
		{"True when this building improves another one instead of standing on its own."},
		"Whether this building is an upgrade of another, as a citadel is of a fort.");
	R.method<&CBuilding::getNameTranslated>("getNameTranslated",
		{"Name of this building in the player's language."},
		"Returns the translated name of this building.");
}

std::string BuildingProxy::getJsonKey(const CBuilding & building)
{
	return building.getJsonKey();
}

bool BuildingProxy::isUpgrade(const CBuilding & building)
{
	return building.upgrade != BuildingID::NONE;
}

}
