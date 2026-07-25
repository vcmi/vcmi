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

#include <vcmi/Environment.h>

#include "../lib/callback/IGameEventCallback.h"
#include "../lib/callback/IGameInfoCallback.h"
#include "../lib/json/JsonNode.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/CGObjectInstance.h"
#include "../lib/mapObjects/CGTownInstance.h"

namespace scripting
{

LuaMapEventDispatcher::LuaMapEventDispatcher(std::shared_ptr<const LuaScriptInstance> script, const Environment * env)
	: script(std::move(script))
	, env(env)
	, context(this->script->createContext(env))
{
}

LuaMapEventDispatcher::~LuaMapEventDispatcher() = default;

void LuaMapEventDispatcher::onObjectVisit(IGameEventCallback & server, const std::string & handler,
	const CGObjectInstance * object, const CGHeroInstance * hero)
{
	if(context->hasFunction(handler))
		context->callMethod<void>(handler, JsonNode(), env->game(), &server, object, hero);
}

void LuaMapEventDispatcher::onPlayerTurnStart(IGameEventCallback & server, const std::string & handler, PlayerColor player)
{
	if(context->hasFunction(handler))
		context->callMethod<void>(handler, JsonNode(), env->game(), &server, player);
}

void LuaMapEventDispatcher::onTownTurnStart(IGameEventCallback & server, const std::string & handler, const CGTownInstance * town)
{
	if(context->hasFunction(handler))
		context->callMethod<void>(handler, JsonNode(), env->game(), &server, town);
}

}
