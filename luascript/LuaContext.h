/*
 * LuaScriptingContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "LuaScriptInstance.h"
#include "LuaStack.h"
#include "LuaReference.h"
#include "../lib/json/JsonNode.h"
#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

class LuaReference;

/// Manages a single running Lua state for one script; created per-script by LuaScriptPool and does not survive map restarts.
/// Exposes the VCMI C++ API to the script and dispatches calls to script-defined functions via callMethod.
class LuaContext final : public Context
{
public:
	LuaContext(const LuaScriptInstance * source, const Environment * env_);
	~LuaContext();

	/// Runs script once to perform its initialization
	void initialize();

	/// Returns true if the script table defines a function with the given name
	bool hasFunction(const std::string & name);

	/// Calls a method on the script class using OOP convention.
	/// params is pushed as self (with __index = scriptTable), remaining args follow.
	/// Return value (if any) is converted from Lua and returned.
	/// For void return, use explicit ReturnType = void.
	template<typename ReturnType, typename... Args>
	ReturnType callMethod(const std::string & name, const JsonNode & params, Args&&... args);

private:
	std::mutex mutex;
	lua_State * L;

	const LuaScriptInstance * script;

	const Environment * env;

	std::shared_ptr<LuaReference> modules;
	std::shared_ptr<LuaReference> scriptTable;

	/// Loads one Lua chunk from source. On success the chunk function is at the top of the stack.
	/// Returns false on compile failure with the error string left on the stack.
	bool loadLayerChunk(const std::string & sourceText, const std::string & identifier);

	/// Replaces the environment of the chunk on top of the stack so that the global `Base`
	/// resolves to the given base table, while other globals fall back through __index = _G.
	void installChunkEnvWithBase(LuaReference & base);

	//log error and return nil from LuaCFunction
	int errorRetVoid(const std::string & message);

	std::string toStringRaw(int index);

	void cleanupGlobals();

	void registerPublicTypes();

	//require global function
	static int require(lua_State * L);
	static int luaError(lua_State * L);
	static int luaAssert(lua_State * L);

	/// Custom text printing function for use in scripting
	/// based on luaB_print (part of Lua source code)
	/// adapted to C++ & VCMI logging facilities
	static int luaPrint(lua_State * L);

	//require function implementation
	// Returns 1 on success (module table on stack).
	// Returns -1 on error (error string on stack).
	int loadModule();
};

template<typename ReturnType, typename... Args>
ReturnType LuaContext::callMethod(const std::string & name, const JsonNode & params, Args&&... args)
{
	std::lock_guard guard(mutex);

	if(!scriptTable)
	{
		if constexpr (!std::is_void_v<ReturnType>)
			return ReturnType{};
		else
			return;
	}

	LuaStack S(L);

	scriptTable->push();               // stack: (table)
	lua_getfield(L, -1, name.c_str()); // stack: (table), (function)
	lua_replace(L, 1);                 // stack: (function)

	if(!S.isFunction(-1))
	{
		S.clear();
		logScript->error("Script '%s': function '%s' not found", script->getIdentifier(), name);
		if constexpr (!std::is_void_v<ReturnType>)
			return ReturnType{};
		else
			return;
	}

	// Build self: push params as Lua table, set __index = scriptTable via metatable
	S.push(params);                    // stack: (function), (self)
	lua_newtable(L);                   // stack: (function), (self), (mt)
	scriptTable->push();               // stack: (function), (self), (mt), (scriptTable)
	lua_setfield(L, -2, "__index");    // mt.__index = scriptTable; stack: (function), (self), (mt)
	lua_setmetatable(L, -2);           // setmetatable(self, mt); stack: (function), (self)

	// push all params
	int argc = 1 + sizeof...(Args);
	(S << ... << args);

	if(lua_pcall(L, argc, 1, 0))
	{
		std::string error = lua_tostring(L, -1);
		S.clear();
		logScript->error("Script '%s', function '%s': %s", script->getIdentifier(), name, error);
		if constexpr (!std::is_void_v<ReturnType>)
			return ReturnType{};
		else
			return;
	}

	if constexpr (!std::is_void_v<ReturnType>)
	{
		ReturnType ret{};
		try
		{
			S.get(S.absindex(-1), ret);
		}
		catch(const LuaApiException & e)
		{
			S.clear();
			logScript->error("Script '%s', function '%s' returned unexpected value: %s", script->getIdentifier(), name, e.what());
			return ReturnType{};
		}
		S.restoreInitialTop();
		return ret;
	}
	else
	{
		S.clear();
		return;
	}
}

}

VCMI_LIB_NAMESPACE_END
