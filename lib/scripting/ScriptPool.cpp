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
#include "vcmi/Services.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::vector<std::string> IMPLEMENTS_MAP =
{
	"ANYTHING",
	"BATTLE_EFFECT"
};

namespace scripting
{

ScriptPoolImpl::ScriptPoolImpl(const Environment * ENV)
	: ScriptPoolImpl(ENV, nullptr)
{}

ScriptPoolImpl::ScriptPoolImpl(const Environment * ENV, ServerCallback * SRV)
	: env(ENV),
	srv(SRV)
{
	env->services()->scripts()->initializePool(*this);
}

void ScriptPoolImpl::registerScript(const Script * script)
{
	auto context = script->createContext(env);
	cache[script] = context;

	const auto & key = script->getJsonKey();
	const JsonNode & scriptState = state[key];

	if(srv)
		context->run(srv, scriptState);
	else
		context->run(scriptState);
}

std::shared_ptr<Context> ScriptPoolImpl::getContext(const Script * script) const
{
	return cache.at(script);
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
