/*
 * api/Faction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Faction.h"

#include "EntityBindings.h"
#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void FactionProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<Faction>::registerMethods(R);

	R.method<&Faction::hasTown>("hasTown", {},
		"True if this faction has town belonging to it in the game. Usually true for all factions other than neutral");
}

}

VCMI_LIB_NAMESPACE_END
