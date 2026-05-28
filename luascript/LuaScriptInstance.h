/*
 * LuaScriptInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class JsonSerializeFormat;
class Services;

namespace scripting
{

class LuaModule;
class LuaContext;

class LuaScriptInstance final : public Script
{
public:
	LuaScriptInstance(LuaModule & host, const std::string & modScope, const std::string & sourcePath);
	virtual ~LuaScriptInstance();

	std::string modScope;
	std::string sourcePath;
	std::string sourceText;

	LuaModule & host;

	std::shared_ptr<LuaContext> createContext(const Environment * ENV) const;

	std::string getIdentifier() const override { return modScope + ':' + sourcePath;}
	const std::string & getSource() const override { return sourceText;}
};
}

VCMI_LIB_NAMESPACE_END
