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

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

namespace scripting
{

class LuaModule;
class LuaScriptInstance;

class LuaScriptPool : public Pool
{
public:
	LuaScriptPool(const LuaModule & module, const Environment * ENV);

	std::shared_ptr<Context> getContext(const Script * script) const override;

	void registerScript(const LuaScriptInstance * script);

private:
	std::map<const Script *, std::shared_ptr<Context>> cache;

	const Environment * env;
};
}

VCMI_LIB_NAMESPACE_END
