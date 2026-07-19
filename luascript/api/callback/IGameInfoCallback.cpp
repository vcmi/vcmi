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
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/modding/ModScope.h"

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
	R.function<&IGameInfoCallbackProxy::getMapVariable>("getMapVariable",
		{
			{"name",  "Variable name within the target mod namespace."},
			{"modID", "Mod namespace to read from; defaults to the current map's scope. Pass another mod's id for cross-mod access."}
		},
		{"Stored value, or nil when the variable is unset."},
		"Reads a persistent map script variable.");
	R.function<&IGameInfoCallbackProxy::hasMapVariable>("hasMapVariable",
		{
			{"name",  "Variable name within the target mod namespace."},
			{"modID", "Mod namespace to read from; defaults to the current map's scope."}
		},
		{"True when the variable has been set."},
		"Returns whether a persistent map script variable has been set.");
}

JsonNode IGameInfoCallbackProxy::getMapVariable(const GameCb & object, const std::string & name, const std::optional<std::string> & modID)
{
	return object.gameState().getMap().getScriptVariables().get(modID.value_or(ModScope::scopeMap()), name);
}

bool IGameInfoCallbackProxy::hasMapVariable(const GameCb & object, const std::string & name, const std::optional<std::string> & modID)
{
	return object.gameState().getMap().getScriptVariables().has(modID.value_or(ModScope::scopeMap()), name);
}

}
