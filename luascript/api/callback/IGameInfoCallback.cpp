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
	R.function<&IGameInfoCallbackProxy::playerIsHuman>("playerIsHuman",
		{{"player", "Player color index to check."}},
		{"True when that player is controlled by a human, false for an AI or an unused color."},
		"Tells whether the given player is controlled by a human.");
	R.function<&IGameInfoCallbackProxy::playerDefeated>("playerDefeated",
		{{"player", "Player color index to check."}},
		{"True when that player has been defeated and is out of the game."},
		"Tells whether the given player has already lost the game.");
	R.function<&IGameInfoCallbackProxy::playerStartingFaction>("playerStartingFaction",
		{
			{"player",  "Player color index to check."},
			{"faction", "Town faction JSON key to compare against."}
		},
		{"True when the player began the map with the given town faction."},
		"Tells whether the player's starting town faction matches the given one.");
	R.function<&IGameInfoCallbackProxy::wasQuestProposed>("wasQuestProposed",
		{
			{"target", "The quest source (seer hut / quest guard) to check."},
			{"player", "Player to check."}
		},
		{"True once the player has already been offered the object's active quest."},
		"Tells whether a player has already seen the object's current quest proposed, so a script can show "
		"the progression text instead of the proposal text on a repeat visit.");
	R.function<&IGameInfoCallbackProxy::heroOwner>("heroOwner",
		{
			{"hero",   "Hero type JSON key to look for on the map."},
			{"player", "Player color index to compare ownership against."}
		},
		{"True when a hero of that type is present on the map and owned by the given player."},
		"Tells whether the hero of the given type is currently owned by the given player.");
	R.function<&IGameInfoCallbackProxy::playerOwnsTown>("playerOwnsTown",
		{
			{"player",     "Player color index to check."},
			{"objectName", "Instance name of the town, as resolved through the questObjects table."}
		},
		{"True when the named town exists and belongs to the given player."},
		"Tells whether the given player owns the named town.");
	R.function<&IGameInfoCallbackProxy::playerDefeatedMonster>("playerDefeatedMonster",
		{
			{"player",     "Player color index to check."},
			{"objectName", "Instance name of the monster, as resolved through the questObjects table."}
		},
		{"True when the given player has destroyed the named monster."},
		"Tells whether the given player has defeated the named wandering monster.");
	R.function<&IGameInfoCallbackProxy::playerDefeatedHero>("playerDefeatedHero",
		{
			{"player",     "Player color index to check."},
			{"objectName", "Instance name of the hero, as resolved through the questObjects table."}
		},
		{"True when the given player has destroyed the named hero."},
		"Tells whether the given player has defeated the named hero.");
	R.function<&IGameInfoCallbackProxy::compareDifficulty>("compareDifficulty",
		{{"reference", "Difficulty level to compare against (0 = easiest)."}},
		{"The current difficulty minus the reference: negative if easier, zero if equal, positive if harder."},
		"Compares the current game difficulty against a reference level, returning their signed difference.");
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

bool IGameInfoCallbackProxy::playerDefeated(const GameCb & object, PlayerColor player)
{
	return object.getPlayerStatus(player, false) == EPlayerStatus::LOSER;
}

bool IGameInfoCallbackProxy::playerStartingFaction(const GameCb & object, PlayerColor player, FactionID faction)
{
	const auto & players = object.getStartInfo()->playerInfos;
	auto it = players.find(player);
	return it != players.end() && it->second.castle == faction;
}

bool IGameInfoCallbackProxy::wasQuestProposed(const GameCb & object, const CGObjectInstance & target, PlayerColor player)
{
	const auto * questSource = dynamic_cast<const QuestSource *>(&target);
	const Quest * activeQuest = questSource ? questSource->getActiveQuest() : nullptr;
	return activeQuest && activeQuest->isKnownTo(player);
}

bool IGameInfoCallbackProxy::heroOwner(const GameCb & object, HeroTypeID hero, PlayerColor player)
{
	for(const auto & mapObject : object.gameState().getMap().objects)
	{
		const auto * heroObject = dynamic_cast<const CGHeroInstance *>(mapObject.get());
		if(heroObject && heroObject->getHeroTypeID() == hero)
			return heroObject->getOwner() == player;
	}
	return false;
}

bool IGameInfoCallbackProxy::playerOwnsTown(const GameCb & object, PlayerColor player, const std::string & objectName)
{
	const auto * town = dynamic_cast<const CGTownInstance *>(getObjectByName(object, objectName));
	return town && town->getOwner() == player;
}

bool IGameInfoCallbackProxy::playerDefeatedMonster(const GameCb & object, PlayerColor player, const std::string & objectName)
{
	const CGObjectInstance * target = getObjectByName(object, objectName);
	const auto * state = object.getPlayerState(player, false);
	return target && state && state->destroyedObjects.count(target->id) != 0;
}

bool IGameInfoCallbackProxy::playerDefeatedHero(const GameCb & object, PlayerColor player, const std::string & objectName)
{
	// destroyedObjects is object-type agnostic; a defeated hero and a defeated monster are the same lookup
	return playerDefeatedMonster(object, player, objectName);
}

int IGameInfoCallbackProxy::compareDifficulty(const GameCb & object, int reference)
{
	return static_cast<int>(object.getStartInfo()->difficulty) - reference;
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
