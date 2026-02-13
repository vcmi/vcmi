/*
 * LuaScriptModule.h, part of VCMI engine
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

namespace scripting
{

class LuaScriptInstance;

class LuaModule final : public Module
{
public:
	LuaModule();
	virtual ~LuaModule();

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void afterLoadFinalization() override;

	std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const override;

private:
	using ScriptPtr = std::shared_ptr<LuaScriptInstance>;
	using ScriptMap = std::map<std::string, ScriptPtr>;

	ScriptMap objects;

	ScriptPtr loadFromJson(vstd::CLoggerBase * logger, const std::string & scope, const JsonNode & json, const std::string & identifier);
};
}

VCMI_LIB_NAMESPACE_END
