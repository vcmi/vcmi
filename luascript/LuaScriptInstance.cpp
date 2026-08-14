/*
 * LuaScriptInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaContext.h"
#include "LuaScriptInstance.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/modding/ModScope.h"

namespace scripting
{

LuaScriptInstance::LuaScriptInstance(const LuaModule & host,
	const std::string & baseScope, const ScriptPath & basePath,
	const std::vector<std::pair<std::string, std::string>> & patches)
	: host(host)
	, baseModScope(baseScope)
	, baseSourcePath(basePath.getName())
{
	loadLayer(baseScope, basePath);
	for (const auto & [scope, path] : patches)
		loadLayer(scope, path);
}

LuaScriptInstance::LuaScriptInstance(const LuaModule & host, const std::string & baseScope, std::string sourceText,
	const std::vector<std::string> & builtinLayers)
	: host(host)
	, baseModScope(baseScope)
	, baseSourcePath(":map")
{
	Layer layer;
	layer.sourceText = std::move(sourceText);
	layer.identifier = baseModScope + baseSourcePath;
	layers.push_back(std::move(layer));

	for(const auto & name : builtinLayers)
		loadLayer(ModScope::scopeBuiltin(), name);
}

LuaScriptInstance::~LuaScriptInstance() = default;

void LuaScriptInstance::loadLayer(const std::string & modScope, const std::string & sourcePath)
{
	loadLayer(modScope, ScriptPath::builtinTODO(sourcePath).addPrefix("SCRIPTS/"));
}

void LuaScriptInstance::loadLayer(const std::string & modScope, const ScriptPath & sourcePath)
{
	auto * loader = CResourceHandler::get(modScope);
	if (!loader->existsResource(sourcePath))
	{
		logMod->error("Script layer not found: %s:%s", modScope, sourcePath.getName());
		return;
	}

	Layer layer;
	auto rawData = loader->load(sourcePath)->readAll();
	layer.sourceText = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);
	layer.identifier = modScope + ':' + sourcePath.getName();
	layers.push_back(std::move(layer));
}

std::shared_ptr<LuaContext> LuaScriptInstance::createContext(const Environment * ENV) const
{
	return std::make_shared<LuaContext>(this, ENV);
}

}
