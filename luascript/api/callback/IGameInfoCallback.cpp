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

#include <vcmi/Faction.h>
#include <vcmi/HeroType.h>
#include <vcmi/Player.h>
#include <vcmi/ResourceType.h>

#include "../../LuaCallWrapper.h"

#include "../adventure/HeroInstance.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/CPlayerState.h"
#include "../../../lib/StartInfo.h"
#include "../../../lib/callback/IGameInfoCallback.h"
#include "../../../lib/constants/Enumerations.h"
#include "../../../lib/gameState/CGameState.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGObjectInstance.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapObjects/Quest.h"
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
	R.function<&IGameInfoCallbackProxy::getResource>("getResource",
		{
			{"player",   "Player whose treasury is queried."},
			{"resource", "Resource to read, as returned by Services:getResourceByName."}
		},
		{"Amount of that resource the player currently owns."},
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
	R.function<&IGameInfoCallbackProxy::playerIsHuman>("playerIsHuman",
		{{"player", "Player color index to check."}},
		{"True when that player is controlled by a human, false for an AI or an unused color."},
		"Tells whether the given player is controlled by a human.");
	R.function<&IGameInfoCallbackProxy::getPlayerStatus>("getPlayerStatus",
		{{"player", "Player whose status is queried."}},
		{"Current status; compare against ENUM.PlayerStatus."},
		"Returns whether the player is still playing, has won, or has been defeated.");
	R.function<&IGameInfoCallbackProxy::getPlayerFaction>("getPlayerFaction",
		{{"player", "Player whose starting faction is queried."}},
		{"The town faction the player began the map with, or nil when the player has none."},
		"Returns the town faction a player started the map with.");
	R.function<&IGameInfoCallbackProxy::wasQuestProposed>("wasQuestProposed",
		{
			{"target", "The quest source (seer hut / quest guard) to check."},
			{"player", "Player to check."}
		},
		{"True once the player has already been offered the object's active quest."},
		"Tells whether a player has already seen the object's current quest proposed, so a script can show "
		"the progression text instead of the proposal text on a repeat visit.");
	R.function<&IGameInfoCallbackProxy::getHeroByType>("getHeroByType",
		{{"heroType", "Hero type to look for, as returned by Services:getHeroTypeByName."}},
		{"The hero of that type currently on the map, or nil when no such hero exists."},
		"Finds the hero of the given type placed on the map.");
	R.function<&IGameInfoCallbackProxy::playerDestroyedObject>("playerDestroyedObject",
		{
			{"player", "Player to check."},
			{"target", "Map object to check, e.g. a wandering monster or a hero."}
		},
		{"True when that player destroyed the object."},
		"Tells whether the given player destroyed the given map object.");
	R.function<&IGameInfoCallbackProxy::getObjectByName>("getObjectByName",
		{{"objectName", "Instance name of the object, as resolved through the questObjects table."}},
		{"The map object with that instance name, or nil when the name is empty or unknown."},
		"Looks up a map object by its instance name.");
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

bool IGameInfoCallbackProxy::playerIsHuman(const GameCb & object, PlayerColor player)
{
	const auto * state = object.getPlayerState(player, false);
	return state && state->isHuman();
}

EPlayerStatus IGameInfoCallbackProxy::getPlayerStatus(const GameCb & object, PlayerColor player)
{
	return object.getPlayerStatus(player, false);
}

int IGameInfoCallbackProxy::getResource(const GameCb & object, PlayerColor player, const ResourceType & resource)
{
	return object.getResource(player, resource.getId());
}

const Faction * IGameInfoCallbackProxy::getPlayerFaction(const GameCb & object, PlayerColor player)
{
	const auto & players = object.getStartInfo()->playerInfos;
	auto it = players.find(player);
	if(it == players.end() || !it->second.castle.hasValue())
		return nullptr;

	return it->second.castle.toEntity(LIBRARY);
}

bool IGameInfoCallbackProxy::wasQuestProposed(const GameCb & object, const CGObjectInstance & target, PlayerColor player)
{
	const auto * questSource = dynamic_cast<const QuestSource *>(&target);
	const Quest * activeQuest = questSource ? questSource->getActiveQuest() : nullptr;
	return activeQuest && activeQuest->isKnownTo(player);
}

const CGHeroInstance * IGameInfoCallbackProxy::getHeroByType(const GameCb & object, const HeroType & heroType)
{
	for(const auto & mapObject : object.gameState().getMap().objects)
	{
		const auto * heroObject = dynamic_cast<const CGHeroInstance *>(mapObject.get());
		if(heroObject && heroObject->getHeroTypeID() == heroType.getId())
			return heroObject;
	}
	return nullptr;
}

bool IGameInfoCallbackProxy::playerDestroyedObject(const GameCb & object, PlayerColor player, const CGObjectInstance & target)
{
	const auto * state = object.getPlayerState(player, false);
	return state && state->destroyedObjects.count(target.id) != 0;
}

const CGObjectInstance * IGameInfoCallbackProxy::getObjectByName(const GameCb & object, const std::string & objectName)
{
	if(objectName.empty())
		return nullptr;

	const auto & names = object.gameState().getMap().instanceNames;
	auto it = names.find(objectName);
	return it != names.end() ? it->second.get() : nullptr;
}

}
