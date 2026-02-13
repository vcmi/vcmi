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

VCMI_LIB_NAMESPACE_BEGIN

static const std::vector<std::string> IMPLEMENTS_MAP =
{
	"ANYTHING",
	"BATTLE_EFFECT"
};

namespace scripting
{

LuaScriptPool::LuaScriptPool(const LuaModule & module, const Environment * ENV)
	: env(ENV)
{
}

void LuaScriptPool::registerScript(const LuaScriptInstance * script)
{
	auto context = script->createContext(env);
	cache[script] = context;
	context->run({});
}

std::shared_ptr<Context> LuaScriptPool::getContext(const Script * script) const
{
	return cache.at(script);
}
}

VCMI_LIB_NAMESPACE_END
