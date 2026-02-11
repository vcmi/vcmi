/*
 * ScriptPool.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#if SCRIPTING_ENABLED
#include <vcmi/scripting/Service.h>
#include "json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace scripting
{

class DLL_LINKAGE ScriptPoolImpl : public Pool
{
public:
	ScriptPoolImpl(const Environment * ENV);
	ScriptPoolImpl(const Environment * ENV, ServerCallback * SRV);
	std::shared_ptr<Context> getContext(const Script * script) override;

	void serializeState(const bool saving, JsonNode & data) override;

private:
	std::map<const Script *, std::shared_ptr<Context>> cache;

	JsonNode state;

	const Environment * env;
	ServerCallback * srv;
};
}

VCMI_LIB_NAMESPACE_END
#endif
