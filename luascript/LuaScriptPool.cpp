/*
 * LuaScriptPool.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LuaScriptPool.h"

#include "LuaScriptInstance.h"
#include "LuaContext.h"

#include "../lib/json/JsonNode.h"

namespace scripting
{

LuaScriptPool::LuaScriptPool(const LuaModule & luaModule, const Environment * ENV)
	: env(ENV)
{
}

LuaScriptPool::~LuaScriptPool() = default;

void LuaScriptPool::registerScript(const LuaScriptInstance * script)
{
	scripts[script] = script;

	// Build the state of the registering thread right away so that a broken script is reported
	// on session start, as before, instead of on first use in the middle of a game
	getContext(script);
}

std::shared_ptr<Context> LuaScriptPool::getContext(const Script * script) const
{
	auto & threadContexts = contexts.local();

	auto it = threadContexts.find(script);
	if(it == threadContexts.end())
	{
		auto context = scripts.at(script)->createContext(env);
		context->initialize();
		it = threadContexts.emplace(script, std::move(context)).first;
	}

	return it->second;
}
}
