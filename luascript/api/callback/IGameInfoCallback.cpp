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

#include "../../../lib/StartInfo.h"
#include "../../../lib/callback/IGameInfoCallback.h"
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapping/MapDifficulty.h"
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
	R.method<&GameCb::getResource>("getResource",
		{
			{"player",   "Player color index whose treasury is queried."},
			{"resource", "Resource JSON key (e.g. \"wood\", \"gold\")."}
		}, {},
		"Returns the amount of the given resource owned by the player.");
	R.function<&IGameInfoCallbackProxy::getCalendar>("getCalendar",
		{"Calendar for the current in-game date."},
		"Returns the calendar object for the current in-game date.");
	R.function<&IGameInfoCallbackProxy::getDifficulty>("getDifficulty",
		{"Current game difficulty; compare against ENUM.Difficulty (pawn..king)."},
		"Returns the current game difficulty level.");
	R.function<&IGameInfoCallbackProxy::getMapVariable>("getMapVariable",
		{
			{"name",  "Name of the variable to read, as stored with AdventureServer:setMapVariable."}
		},
		{"The stored value, or nil when nothing was ever stored under that name."},
		"Reads back a named value saved earlier with AdventureServer:setMapVariable. "
		"Values survive saving and loading, so this is how a script remembers state between visits.");
	R.function<&IGameInfoCallbackProxy::hasMapVariable>("hasMapVariable",
		{
			{"name",  "Name of the variable to check."}
		},
		{"True when a value has been stored under that name."},
		"Checks whether a named map variable has ever been set, which lets a script tell \"unset\" apart from a stored value of 0 or false.");
}

JsonNode IGameInfoCallbackProxy::getMapVariable(const GameCb & object, const std::string & name)
{
	return object.gameState().getMap().getScriptVariables().get(ModScope::scopeMap(), name);
}

bool IGameInfoCallbackProxy::hasMapVariable(const GameCb & object, const std::string & name)
{
	return object.gameState().getMap().getScriptVariables().has(ModScope::scopeMap(), name);
}

Calendar IGameInfoCallbackProxy::getCalendar(const GameCb & object)
{
	return object.getCalendar();
}

EMapDifficulty IGameInfoCallbackProxy::getDifficulty(const GameCb & object)
{
	return static_cast<EMapDifficulty>(object.getStartInfo()->difficulty);
}

}
