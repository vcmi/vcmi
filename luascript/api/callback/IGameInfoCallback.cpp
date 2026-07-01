/*
 * IGameInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "IGameInfoCallback.h"

#include <vcmi/Player.h>

#include "../../LuaCallWrapper.h"

#include "../adventure/HeroInstance.h"

#include "../../../lib/callback/IGameInfoCallback.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void IGameInfoCallbackProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&GameCb::getHero>("getHero",
		{{"objectID", "Map object identifier of the hero to fetch."}}, {},
		"Returns the hero by its object identifier, or nil if not found.");
	R.method<&GameCb::getObj>("getObj",
		{
			{"objectID", "Map object identifier of the object to fetch."},
			{"verbose",  "Pass true to log a warning when the object isn't found."}
		}, {},
		"Returns the map object by its identifier, or nil if not found.");
}

}

VCMI_LIB_NAMESPACE_END
