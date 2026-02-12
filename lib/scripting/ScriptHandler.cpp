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

	if(!boost::filesystem::exists(filePath))
		throw std::runtime_error("Critical error! Failed to initialize Lua scripting module!");

	lua = CDynLibHandler::getNewScriptingModule(filePath);
}

ScriptHandler::~ScriptHandler() = default;

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	return lua->loadObject(scope, name, data);
}

void ScriptHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	throw std::runtime_error("No legacy data load allowed for scripts");
}

void ScriptHandler::afterLoadFinalization()
{
	return lua->afterLoadFinalization();
}

std::unique_ptr<Pool> ScriptHandler::createPoolInstance(const Environment * ENV) const
{
	return lua->createPoolInstance(ENV);
}

std::vector<JsonNode> ScriptHandler::loadLegacyData()
{
	return std::vector<JsonNode>();
}

}

VCMI_LIB_NAMESPACE_END
