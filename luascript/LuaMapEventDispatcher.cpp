/*
 * LuaMapEventDispatcher.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaMapEventDispatcher.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"

#include "api/adventure/MapScriptInit.h"

#include <vcmi/Environment.h>

#include "../lib/callback/IGameEventCallback.h"
#include "../lib/callback/IGameInfoCallback.h"
#include "../lib/json/JsonNode.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGObjectInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"

namespace scripting
{

static const std::string INIT = "init";

LuaMapEventDispatcher::LuaMapEventDispatcher(std::shared_ptr<const LuaScriptInstance> script, const Environment * env, CMap & map, bool runInit)
	: script(std::move(script))
	, env(env)
	, context(this->script->createContext(env))
{
	context->initialize(); // runs the script chunk so its handler table becomes available

	if(runInit && context->hasFunction(INIT))
	{
		api::MapScriptInit setup(map);
		context->callMethod<void>(INIT, JsonNode(), &setup);
	}
}

LuaMapEventDispatcher::~LuaMapEventDispatcher() = default;

// The coroutine runtime returns 0 when a handler ran to completion, or a positive handle when it
// paused on a blocking action and must be resumed later.
static std::optional<int> asHandle(int result)
{
	return result == 0 ? std::nullopt : std::optional<int>(result);
}

std::optional<int> LuaMapEventDispatcher::onObjectVisit(IGameEventCallback & server, const std::string & handler,
	const CGObjectInstance * object, const CGHeroInstance * hero)
{
	if(!context->hasFunction(handler))
		return std::nullopt;

	// The visited object doubles as the combat host if the handler calls startCombat.
	return asHandle(context->callFunction<int>("run", handler, hero->getOwner(), object, env->game(), &server, object, hero));
}

std::optional<int> LuaMapEventDispatcher::onPlayerTurnStart(IGameEventCallback & server, const std::string & handler, PlayerColor player)
{
	if(!context->hasFunction(handler))
		return std::nullopt;

	// Player turn events carry no object, so there is no combat host.
	return asHandle(context->callFunction<int>("run", handler, player, static_cast<const CGObjectInstance *>(nullptr), env->game(), &server, player));
}

std::optional<int> LuaMapEventDispatcher::onTownTurnStart(IGameEventCallback & server, const std::string & handler, const CGTownInstance * town)
{
	if(!context->hasFunction(handler))
		return std::nullopt;

	// Town turn events do not host combat.
	return asHandle(context->callFunction<int>("run", handler, town->getOwner(), static_cast<const CGObjectInstance *>(nullptr), env->game(), &server, town));
}

bool LuaMapEventDispatcher::resumeCoroutine(IGameEventCallback & server, int coroutineHandle, std::optional<int> answer)
{
	return context->callFunction<int>("resume", coroutineHandle, answer) == 0;
}

}
