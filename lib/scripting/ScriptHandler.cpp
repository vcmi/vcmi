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
#include "ScriptImpl.h"

#if SCRIPTING_ENABLED
#include "../VCMIDirs.h"
#include "../callback/CDynLibHandler.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

ScriptHandler::ScriptHandler()
{
	boost::filesystem::path filePath = VCMIDirs::get().fullLibraryPath("scripting", "vcmiERM");

	filePath = VCMIDirs::get().fullLibraryPath("scripting", "vcmiLua");

	if(boost::filesystem::exists(filePath))
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

std::vector<JsonNode> ScriptHandler::loadLegacyData()
{
	return std::vector<JsonNode>();
}

ScriptPtr ScriptHandler::loadFromJson(vstd::CLoggerBase * logger, const std::string & scope, const JsonNode & json, const std::string & identifier) const
{
	auto ret = std::make_shared<ScriptImpl>(this);

	JsonDeserializer handler(nullptr, json);
	ret->identifier = identifier;
	ret->modScope = scope;
	ret->serializeJson(logger, handler);
	return ret;
}

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(logMod, scope, data, name);
	objects[object->getJsonKey()] = object;
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

		auto script = std::make_shared<ScriptImpl>(this);

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
