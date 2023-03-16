/*
 * ScriptHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ScriptHandler.h"

#if SCRIPTING_ENABLED
#include <vcmi/Services.h>
#include <vcmi/Environment.h>

#include "CGameInterface.h"
#include "CScriptingModule.h"
#include "CModHandler.h"

#include "VCMIDirs.h"
#include "serializer/JsonDeserializer.h"
#include "serializer/JsonSerializer.h"
#include "filesystem/Filesystem.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::vector<std::string> IMPLEMENTS_MAP =
{
	"ANYTHING",
	"BATTLE_EFFECT"
};

namespace scripting
{

ScriptImpl::ScriptImpl(const ScriptHandler * owner_): 
	owner(owner_),
	implements(Implements::ANYTHING)
{
}

ScriptImpl::~ScriptImpl() = default;

void ScriptImpl::compile(vstd::CLoggerBase * logger)
{
	code = host->compile(sourcePath, sourceText, logger);

	if(host == owner->erm)
	{
		host = owner->lua;
		sourceText = code;
		code = host->compile(getName(), getSource(), logger);
	}
}

std::shared_ptr<Context> ScriptImpl::createContext(const Environment * env) const
{
	return host->createContextFor(this, env);
}

const std::string & ScriptImpl::getName() const
{
	return identifier;
}

const std::string & ScriptImpl::getSource() const
{
	return sourceText;
}

void ScriptImpl::performRegistration(Services * services) const
{
	switch(implements)
	{
	case Implements::ANYTHING:
		break;
	case Implements::BATTLE_EFFECT:
		host->registerSpellEffect(services->spellEffects(), this);
		break;
	}
}

void ScriptImpl::serializeJson(vstd::CLoggerBase * logger, JsonSerializeFormat & handler)
{
	handler.serializeString("source", sourcePath);
	handler.serializeEnum("implements", implements, Implements::ANYTHING, IMPLEMENTS_MAP);

	if(!handler.saving)
	{
		resolveHost();

		ResourceID sourcePathId("SCRIPTS/" + sourcePath);

		auto rawData = CResourceHandler::get()->load(sourcePathId)->readAll();

		sourceText = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);

		compile(logger);
	}
}

void ScriptImpl::serializeJsonState(JsonSerializeFormat & handler)
{
	handler.serializeString("sourcePath", sourcePath);
	handler.serializeString("sourceText", sourceText);
	handler.serializeString("code", code);
	handler.serializeEnum("implements", implements, Implements::ANYTHING, IMPLEMENTS_MAP);

	if(!handler.saving)
	{
		host = owner->lua;
	}
}

void ScriptImpl::resolveHost()
{
	ResourceID sourcePathId(sourcePath);

	if(sourcePathId.getType() == EResType::ERM)
		host = owner->erm;
	else if(sourcePathId.getType() == EResType::LUA)
		host = owner->lua;
	else
		throw std::runtime_error("Unknown script language in:" + sourcePath);
}

PoolImpl::PoolImpl(const Environment * ENV)
	: env(ENV),
	srv(nullptr)
{
}

PoolImpl::PoolImpl(const Environment * ENV, ServerCallback * SRV)
	: env(ENV),
	srv(SRV)
{
}

std::shared_ptr<Context> PoolImpl::getContext(const Script * script)
{
	auto iter = cache.find(script);

	if(iter == cache.end())
	{
		auto context = script->createContext(env);
		cache[script] = context;

		const auto & key = script->getName();
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

void PoolImpl::serializeState(const bool saving, JsonNode & data)
{
	if(saving)
	{
        for(auto & scriptAndContext : cache)
		{
			const auto * script = scriptAndContext.first;
			auto context = scriptAndContext.second;

			state[script->getName()] = context->saveState();

			data = state;
		}
	}
	else
	{
		state = data;
	}
}

ScriptHandler::ScriptHandler()
	:erm(nullptr), lua(nullptr)
{
	boost::filesystem::path filePath = VCMIDirs::get().fullLibraryPath("scripting", "vcmiERM");

	if (boost::filesystem::exists(filePath))
	{
		erm = CDynLibHandler::getNewScriptingModule(filePath);
	}

	filePath = VCMIDirs::get().fullLibraryPath("scripting", "vcmiLua");

	if (boost::filesystem::exists(filePath))
	{
		lua = CDynLibHandler::getNewScriptingModule(filePath);
	}
}

ScriptHandler::~ScriptHandler() = default;

const Script * ScriptHandler::resolveScript(const std::string & name) const
{
	auto iter = objects.find(name);

	if(iter == objects.end())
	{
		logMod->error("Unknown script id '%s'", name);
		return nullptr;
	}
	else
	{
		return iter->second.get();
	}
}

std::vector<bool> ScriptHandler::getDefaultAllowed() const
{
	return std::vector<bool>();
}

std::vector<JsonNode> ScriptHandler::loadLegacyData(size_t dataSize)
{
	return std::vector<JsonNode>();
}

ScriptPtr ScriptHandler::loadFromJson(vstd::CLoggerBase * logger, const std::string & scope,
	const JsonNode & json, const std::string & identifier) const
{
	ScriptPtr ret = std::make_shared<ScriptImpl>(this);

	JsonDeserializer handler(nullptr, json);
	ret->identifier = identifier;
	ret->serializeJson(logger, handler);
	return ret;
}

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(logMod, scope, data, name);
	objects[object->identifier] = object;
}

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("No legacy data load allowed for scripts");
}

void ScriptHandler::performRegistration(Services * services) const
{
	for(const auto & keyValue : objects)
	{
		auto script = keyValue.second;
		script->performRegistration(services);
	}
}

void ScriptHandler::run(std::shared_ptr<Pool> pool) const
{
	for(const auto & keyValue : objects)
	{
		auto script = keyValue.second;

		if(script->implements == ScriptImpl::Implements::ANYTHING)
		{
			auto context = pool->getContext(script.get());
			//todo: consider explicit run for generic scripts
		}
	}
}

void ScriptHandler::loadState(const JsonNode & state)
{
	objects.clear();

	const JsonNode & scriptsData = state["scripts"];

	for(const auto & keyValue : scriptsData.Struct())
	{
		std::string name = keyValue.first;

		const JsonNode & scriptData = keyValue.second;

		ScriptPtr script = std::make_shared<ScriptImpl>(this);

		JsonDeserializer handler(nullptr, scriptData);
		script->serializeJsonState(handler);
		objects[name] = script;
	}
}

void ScriptHandler::saveState(JsonNode & state)
{
	JsonNode & scriptsData = state["scripts"];

	for(auto & keyValue : objects)
	{
		std::string name = keyValue.first;

		ScriptPtr script = keyValue.second;
		JsonNode scriptData;
		JsonSerializer handler(nullptr, scriptData);
		script->serializeJsonState(handler);

		scriptsData[name] = scriptData;
	}

}


}

VCMI_LIB_NAMESPACE_END
#endif
