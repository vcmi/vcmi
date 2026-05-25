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

	void run() override;

	JsonNode callGlobal(const std::string & name, const JsonNode & parameters) override;

	template<typename ReturnType, typename... Args>
	ReturnType callGlobalWithParameters(const std::string & name, Args&& ... parameters);

private:
	std::mutex mutex;
	lua_State * L;

	const Script * script;

	const Environment * env;

	std::shared_ptr<LuaReference> modules;
	std::shared_ptr<LuaReference> scriptClosure;

	//log error and return nil from LuaCFunction
	int errorRetVoid(const std::string & message);

	void getGlobal(const std::string & name, int & value) override;
	void getGlobal(const std::string & name, std::string & value) override;
	void getGlobal(const std::string & name, double & value) override;
	void getGlobal(const std::string & name, JsonNode & value) override;

	void setGlobal(const std::string & name, int value) override;
	void setGlobal(const std::string & name, const std::string & value) override;
	void setGlobal(const std::string & name, double value) override;
	void setGlobal(const std::string & name, const JsonNode & value) override;

	void pop(JsonNode & value);

	void popAll();

	void push(const std::string & value);
	void push(lua_CFunction f, void * opaque);

	std::string toStringRaw(int index);

	void cleanupGlobals();

	void registerCore();

	//require global function
	static int require(lua_State * L);

	//require function implementation
	int loadModule();
};

template<typename ReturnType, typename... Args>
ReturnType LuaContext::callGlobalWithParameters(const std::string & name, Args&& ... parameters)
{
	std::lock_guard guard(mutex);
	LuaStack S(L);

	lua_getglobal(L, name.c_str());

	if(!S.isFunction(-1))
	{
		S.clear();
		std::string error = "Function with name " + name + " was not found";
		logGlobal->error(error);
		throw LuaApiException(error);
	}

	int argc = sizeof...(Args);
	(S << ... << parameters);

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
		S.balance();
		return;
	}
}

}

VCMI_LIB_NAMESPACE_END
