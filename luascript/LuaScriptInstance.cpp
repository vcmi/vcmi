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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

LuaScriptInstance::LuaScriptInstance(LuaModule & host, const std::string & modScope, const std::string & sourcePath)
	: host(host)
	, modScope(modScope)
	, sourcePath(sourcePath)
{
	ScriptPath sourcePathId = ScriptPath::builtinTODO(sourcePath).addPrefix("SCRIPTS/");
	CResourceHandler::get(modScope)->load(sourcePathId);

	auto rawData = CResourceHandler::get()->load(sourcePathId)->readAll();
	sourceText = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);
}

LuaScriptInstance::~LuaScriptInstance() = default;

std::shared_ptr<LuaContext> LuaScriptInstance::createContext(const Environment * ENV) const
{
	return std::make_shared<LuaContext>(this, ENV);
}

}

VCMI_LIB_NAMESPACE_END
