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
	R.function<&MapObjectProxy::getOwner>("getOwner",
		{"Player color index that owns this object (neutral index when unowned)."},
		"Returns the owner of this map object.");
}

int MapObjectProxy::getOwner(const CGObjectInstance & object)
{
	return object.getOwner().getNum();
}

}
