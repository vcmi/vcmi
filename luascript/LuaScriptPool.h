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

#include <tbb/enumerable_thread_specific.h>

class JsonNode;

namespace scripting
{

class LuaContext;
class LuaModule;
class LuaScriptInstance;

/// Owned by CGameState; hands out the LuaContext of every registered script in the current game session.
/// Scripts are registered at session start; each thread gets its own set of contexts so that scripts can
/// run concurrently without locking. This requires scripts to be stateless - they receive their
/// configuration as a per-call argument and must not accumulate state in globals or in their own table.
class LuaScriptPool : public Pool
{
public:
	LuaScriptPool(const LuaModule & luaModule, const Environment * ENV);
	~LuaScriptPool();

	std::shared_ptr<Context> getContext(const Script * script) const override;

	void registerScript(const LuaScriptInstance * script);

private:
	/// Filled at session start and never modified afterwards, so concurrent lookup needs no locking
	std::map<const Script *, const LuaScriptInstance *> scripts;

	/// Per-thread lua_State of each script, created on first use by that thread
	mutable tbb::enumerable_thread_specific<std::map<const Script *, std::shared_ptr<LuaContext>>> contexts;

	const Environment * env;
};
}
