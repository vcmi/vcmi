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

}
