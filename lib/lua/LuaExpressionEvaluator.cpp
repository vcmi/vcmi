/*
 * LuaExpressionEvaluator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LuaExpressionEvaluator.h"

VCMI_LIB_NAMESPACE_BEGIN

std::mutex LuaExpressionEvaluator::mutex;

void LuaExpressionEvaluator::registerLibrary()
{
	const auto & luaMax = [](lua_State * state)
	{
		lua_Number a = luaL_checknumber(state, 1);
		lua_Number b = luaL_checknumber(state, 2);
		lua_pushnumber(state, std::max(a,b));
		return 1;
	};

	const auto & luaMin = [](lua_State * state)
	{
		lua_Number a = luaL_checknumber(state, 1);
		lua_Number b = luaL_checknumber(state, 2);
		lua_pushnumber(state, std::min(a,b));
		return 1;
	};

	const auto & luaClamp = [](lua_State * state)
	{
		lua_Number a = luaL_checknumber(state, 1);
		lua_Number b = luaL_checknumber(state, 2);
		lua_Number c = luaL_checknumber(state, 3);
		lua_pushnumber(state, std::clamp(a,b,c));
		return 1;
	};

	const luaL_Reg registry[] = {
			{ "max", luaMax },
			{ "min", luaMin },
			{ "clamp", luaClamp },
			{ nullptr,	nullptr }
	};

	lua_pushvalue(luaState, LUA_GLOBALSINDEX);
	luaL_setfuncs(luaState,registry,0);
}

LuaExpressionEvaluator::LuaExpressionEvaluator(const std::string & expression)
		: expression(expression)
{
	std::lock_guard<std::mutex> lock(mutex);
	luaState = luaL_newstate();
	registerLibrary();
	compile();
}

LuaExpressionEvaluator::~LuaExpressionEvaluator()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (luaState)
	{
		lua_close(luaState);
		luaState = nullptr;
	}
}

void LuaExpressionEvaluator::compile()
{
	int result = luaL_loadstring(luaState, expression.c_str());

	if (result)
	{
		logGlobal->error("Lua compilation failure: %s", lua_tostring(luaState,-1));
		lua_pop(luaState,1);
	}

	assert(result == LUA_OK);
	compiledReference = luaL_ref(luaState, LUA_REGISTRYINDEX);
}

double LuaExpressionEvaluator::evaluate(const std::map<std::string, double>& param)
{
	std::lock_guard<std::mutex> lock(mutex);
	for (auto &elem : param)
	{
		lua_pushnumber(luaState, elem.second);
		lua_setglobal(luaState, elem.first.c_str());
	}

	lua_rawgeti(luaState, LUA_REGISTRYINDEX, compiledReference);
	int errorCode = lua_pcall(luaState,0,1,0);
	if (errorCode)
	{
		logGlobal->error("Lua evaluation failure: %s", lua_tostring(luaState,-1));
		lua_pop(luaState,1);
		return 0;
	}

	double result = lua_tonumber(luaState,-1);
	lua_pop(luaState,1);
	return result;
}

VCMI_LIB_NAMESPACE_END