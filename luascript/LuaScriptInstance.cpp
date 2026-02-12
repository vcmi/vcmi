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
#include "../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::vector<std::string> IMPLEMENTS_MAP = {"ANYTHING", "BATTLE_EFFECT"};

namespace scripting
{

LuaScriptInstance::LuaScriptInstance(LuaModule & host) : host(host), implements(Implements::ANYTHING) {}

LuaScriptInstance::~LuaScriptInstance() = default;

std::string LuaScriptInstance::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::shared_ptr<LuaContext> LuaScriptInstance::createContext(const Environment * ENV) const
{
	return std::make_shared<LuaContext>(this, ENV);
}

const std::string & LuaScriptInstance::getSource() const
{
	return sourceText;
}

void LuaScriptInstance::loadFromJson(const JsonNode & input)
{
	sourcePath = input["source"].String();
	implements = static_cast<Implements>(vstd::find_pos(IMPLEMENTS_MAP, input["implements"].String()));
	ResourcePath sourcePathId(sourcePath);
	auto rawData = CResourceHandler::get()->load(sourcePathId)->readAll();
	sourceText = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);
}

}

VCMI_LIB_NAMESPACE_END
