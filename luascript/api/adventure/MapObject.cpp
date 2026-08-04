/*
 * MapObject.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "MapObject.h"

#include "../../LuaCallWrapper.h"

namespace scripting::api
{

void MapObjectProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&CGObjectInstance::getOwner>("getOwner",
		{"Player that owns this object, or the neutral player when unowned."},
		"Returns the owner of this map object.");
	R.function<&MapObjectProxy::getInstanceName>("getInstanceName",
		{"The object's unique instance name, as set in the map editor or auto-generated."},
		"Returns the map-unique instance name identifying this object.");
}

std::string MapObjectProxy::getInstanceName(const CGObjectInstance & object)
{
	return object.instanceName;
}

}
