/*
 * IGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

#include "../../../lib/callback/IGameInfoCallback.h"

#include <vcmi/scripting/Service.h>

class CGObjectInstance;
class CGHeroInstance;
class Faction;
class HeroType;
class ResourceType;
class JsonNode;
enum class EMapDifficulty : uint8_t;

namespace scripting::api
{

class IGameInfoCallbackProxy : public RawPointerWrapper<const GameCb, IGameInfoCallbackProxy>
{
public:
	static constexpr std::string_view luaName = "Game";
	static constexpr std::string_view luaDescription =
		"Adventure-map query interface. Provides world-level "
		"lookups: current date, players, towns, heroes, and map objects accessible to the "
		"calling script's owner.";

	static void registerMethods(MethodRegistrar & R);

	static JsonNode getMapVariable(const GameCb & object, const std::string & name);
	static bool hasMapVariable(const GameCb & object, const std::string & name);
	static Calendar getCalendar(const GameCb & object);
	static EMapDifficulty getDifficulty(const GameCb & object);
	static bool playerIsHuman(const GameCb & object, PlayerColor player);
	static EPlayerStatus getPlayerStatus(const GameCb & object, PlayerColor player);
	static int getResource(const GameCb & object, PlayerColor player, const ResourceType & resource);
	static const Faction * getPlayerFaction(const GameCb & object, PlayerColor player);
	static bool wasQuestProposed(const GameCb & object, const CGObjectInstance & target, PlayerColor player);
	static const CGHeroInstance * getHeroByType(const GameCb & object, const HeroType & heroType);
	static bool playerDestroyedObject(const GameCb & object, PlayerColor player, const CGObjectInstance & target);
	/// Resolves a map object by its instance name, or nullptr when the name is empty or unknown.
	static const CGObjectInstance * getObjectByName(const GameCb & object, const std::string & objectName);
};

}
