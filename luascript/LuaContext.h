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

#include "LuaStack.h"
#include "LuaReference.h"
#include "../lib/json/JsonNode.h"
#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

class LuaReference;

class LuaContext final : public Context
{
public:
	LuaContext(const Script * source, const Environment * env_);
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

	const Script * script;

	const Environment * env;

	std::shared_ptr<LuaReference> modules;
	std::shared_ptr<LuaReference> scriptClosure;
	std::shared_ptr<LuaReference> scriptTable;

	//log error and return nil from LuaCFunction
	int errorRetVoid(const std::string & message);

	void popAll();

	void push(const std::string & value);
	void push(lua_CFunction f, void * opaque);

	std::string toStringRaw(int index);

	void cleanupGlobals();

	void registerPublicTypes();

	//require global function
	static int require(lua_State * L);

	//require function implementation
	int loadModule();
};

template<typename ReturnType, typename... Args>
ReturnType LuaContext::callMethod(const std::string & name, const JsonNode & params, Args&&... args)
{
	std::lock_guard guard(mutex);
	LuaStack S(L);

	scriptTable->push();               // stack: (table)
	lua_getfield(L, -1, name.c_str()); // stack: (table), (function)
	lua_replace(L, 1);                 // stack: (function)

	if(!S.isFunction(-1))
	{
		S.clear();
		std::string error = "Function with name " + name + " was not found";
		logGlobal->error(error);
		throw LuaApiException(error);
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

		boost::format fmt("Lua function %s failed with message: %s");
		fmt % name % error;
		logGlobal->error(fmt.str());
		throw LuaApiException(error);
	}

	if constexpr (!std::is_void_v<ReturnType>)
	{
		ReturnType ret;
		S.getOrThrow(S.absindex(-1), ret);
		S.balance();
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
