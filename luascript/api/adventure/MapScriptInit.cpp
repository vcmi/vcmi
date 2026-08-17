/*
 * MapScriptInit.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "MapScriptInit.h"

#include "../../LuaCallWrapper.h"
#include "../../LuaStack.h"

#include "../../../lib/mapObjects/CGObjectInstance.h"
#include "../../../lib/mapObjects/CGPandoraBox.h"
#include "../../../lib/mapping/CMap.h"

namespace scripting::api
{

void MapScriptInitProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&MapScriptInitProxy::attachEventScript>("attachEventScript",
		{
			{"funcName",   "Name of the script function to run when the object is visited."},
			{"objectName", "Instance name of the event/pandora object to bind the handler to."}
		}, {},
		"Binds one of the script's handler functions to a map event object: visiting that object then runs "
		"the named function (with the usual game, server, object, hero arguments) instead of the object's "
		"default reward. Errors if no object has that instance name or the object is not an event/pandora.");
	R.function<&MapScriptInitProxy::objects>("objects",
		{"Every object currently placed on the map."},
		"Returns all objects on the map so the script can find the ones to attach handlers to (match on getInstanceName).");
}

void MapScriptInitProxy::attachEventScript(MapScriptInit & object, const std::string & funcName, const std::string & objectName)
{
	auto it = object.map.instanceNames.find(objectName);
	if(it == object.map.instanceNames.end())
		throw LuaApiException("attachEventScript: no map object named '" + objectName + "'");

	auto * pandora = dynamic_cast<CGPandoraBox *>(it->second.get());
	if(!pandora)
		throw LuaApiException("attachEventScript: object '" + objectName + "' is not an event/pandora object");

	pandora->heroVisitScriptHandler = funcName;
}

std::vector<const CGObjectInstance *> MapScriptInitProxy::objects(const MapScriptInit & object)
{
	std::vector<const CGObjectInstance *> result;
	for(const auto & obj : object.map.objects)
		if(obj)
			result.push_back(obj.get());
	return result;
}

}
