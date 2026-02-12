/*
 * ScriptImpl.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ScriptDefines.h"

#include "../IHandlerBase.h"
#include "../json/JsonNode.h"

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;
class Services;

namespace scripting
{

class DLL_LINKAGE ScriptImpl : public Script
{
public:
	enum class Implements
	{
		ANYTHING,
		BATTLE_EFFECT
		//todo: adventure effect, map object(unified with building), server query, client query(with gui), may be smth else
	};

	Implements implements;

	std::string identifier;
	std::string modScope;
	std::string sourcePath;
	std::string sourceText;

	std::string code;

	ModulePtr host;

	ScriptImpl(const ScriptHandler * owner_);
	virtual ~ScriptImpl();

	void compile(vstd::CLoggerBase * logger);

	void serializeJson(vstd::CLoggerBase * logger, JsonSerializeFormat & handler);
	void serializeJsonState(JsonSerializeFormat & handler);

	std::shared_ptr<Context> createContext(const Environment * env) const override;
	std::string getJsonKey() const override;
	const std::string & getSource() const override;

	void performRegistration(Services * services) const;

private:
	const ScriptHandler * owner;

	void resolveHost();
};
}

VCMI_LIB_NAMESPACE_END
