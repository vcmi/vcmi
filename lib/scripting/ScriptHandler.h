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

#include "../IHandlerBase.h"

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace scripting
{

class DLL_LINKAGE ScriptHandler final
	: public IHandlerBase
	, public Service
{
public:
	ScriptHandler();
	virtual ~ScriptHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void afterLoadFinalization() override;

	std::unique_ptr<Pool> createPoolInstance(const Environment * ENV) const override;

private:
	std::shared_ptr<Module> lua;
};
}

VCMI_LIB_NAMESPACE_END
