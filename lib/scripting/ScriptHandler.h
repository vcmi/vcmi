/*
 * ScriptHandler.h, part of VCMI engine
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

class Services;

namespace scripting
{

class DLL_LINKAGE ScriptHandler
	: public IHandlerBase
	, public Service
{
public:
	ScriptMap objects;

	ScriptHandler();
	virtual ~ScriptHandler();

	const Script * resolveScript(const std::string & name) const;

	std::vector<JsonNode> loadLegacyData() override;

	ScriptPtr loadFromJson(vstd::CLoggerBase * logger, const std::string & scope, const JsonNode & json, const std::string & identifier) const;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void performRegistration(Services * services) const override;

	void initializePool(Pool & pool) const override;
	void run(Pool & pool) const override;

	template<typename Handler>
	void serialize(Handler & h)
	{
		JsonNode state;
		if(h.saving)
			saveState(state);

		h & state;

		if(!h.saving)
			loadState(state);
	}

	ModulePtr lua;

private:
	friend class ScriptImpl;

	void loadState(const JsonNode & state);
	void saveState(JsonNode & state);
};
}

VCMI_LIB_NAMESPACE_END
