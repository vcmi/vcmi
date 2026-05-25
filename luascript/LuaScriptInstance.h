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

class JsonSerializeFormat;
class Services;

namespace scripting
{

class LuaModule;
class LuaContext;

class LuaScriptInstance : public Script
{
public:
	LuaScriptInstance(LuaModule & host);
	virtual ~LuaScriptInstance();

	enum class Implements : int8_t
	{
		NONE = -1,
		ANYTHING,
		BATTLE_EFFECT
	};

	Implements implements;

	std::string identifier;
	std::string modScope;
	std::string sourcePath;
	std::string sourceText;

	LuaModule & host;

	std::shared_ptr<LuaContext> createContext(const Environment * ENV) const;

	void loadFromJson(const JsonNode & input);

	std::string getJsonKey() const override;
	const std::string & getSource() const override;
};
}

VCMI_LIB_NAMESPACE_END
