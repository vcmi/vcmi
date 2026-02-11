/*
 * ScriptPool.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ScriptPool.h"

#if SCRIPTING_ENABLED

VCMI_LIB_NAMESPACE_BEGIN

static const std::vector<std::string> IMPLEMENTS_MAP =
{
	"ANYTHING",
	"BATTLE_EFFECT"
};

namespace scripting
{

ScriptPoolImpl::ScriptPoolImpl(const Environment * ENV)
	: env(ENV),
	srv(nullptr)
{
}

ScriptPoolImpl::ScriptPoolImpl(const Environment * ENV, ServerCallback * SRV)
	: env(ENV),
	srv(SRV)
{
}

std::shared_ptr<Context> ScriptPoolImpl::getContext(const Script * script)
{
	auto iter = cache.find(script);

	if(iter == cache.end())
	{
		auto context = script->createContext(env);
		cache[script] = context;

		const auto & key = script->getJsonKey();
		const JsonNode & scriptState = state[key];

		if(srv)
			context->run(srv, scriptState);
		else
			context->run(scriptState);

		return context;
	}
	else
	{
		return iter->second;
	}
}

void ScriptPoolImpl::serializeState(const bool saving, JsonNode & data)
{
	if(saving)
	{
		for(auto & scriptAndContext : cache)
		{
			const auto * script = scriptAndContext.first;
			auto context = scriptAndContext.second;

			state[script->getJsonKey()] = context->saveState();

			data = state;
		}
	}
	else
	{
		state = data;
	}
}
}

VCMI_LIB_NAMESPACE_END
#endif
