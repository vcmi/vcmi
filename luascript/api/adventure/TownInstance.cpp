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

#include "../../../lib/mapObjects/CGObjectInstance.h"

namespace scripting::api
{

void TownInstanceProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&CGObjectInstance::getOwner, CGTownInstance>("getOwner", {},
		"Returns the player color that owns this town, or the neutral player when it is unowned.");
}

}
