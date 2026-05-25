/*
 * LuaScriptingContext.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "LuaContext.h"

#include "LuaStack.h"
#include "LuaReference.h"

#include "api/Enums.h"

#include "../lib/callback/IGameInfoCallback.h"
#include "../lib/json/JsonNode.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/modding/ModScope.h"

#include <vstd/StringUtils.h>
#include <vcmi/ServerCallback.h>
#include <vcmi/Services.h>

VCMI_LIB_NAMESPACE_BEGIN

/// Custom text printing function for use in scripting
/// based on luaB_print (part of Lua source code)
/// adapted to C++ & VCMI logging facilities
static int luaPrint(lua_State *L) {
	int n = lua_gettop(L);
	lua_getglobal(L, "tostring");
	std::string out;
	for (int i = 1; i <= n; ++i)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		const char *s = lua_tostring(L, -1);
		if (s)
			out += s;
		if (i > 1)
			out += '\t';

		lua_pop(L, 1);
	}

	logScript->info("%s", out);
	return 0;
}

namespace scripting
{

LuaContext::LuaContext(const Script * source, const Environment * env_):
	L(luaL_newstate()),
	script(source),
	env(env_)
{
	static constexpr std::array<luaL_Reg, 4> STD_LIBS =
	{{
		{"_G", luaopen_base},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math}
	}};

	for(const luaL_Reg & lib : STD_LIBS)
	{
		lib.func(L);
		lua_setglobal(L, lib.name);
	}

	popAll();

	cleanupGlobals();

	popAll();

	lua_newtable(L);
	modules = std::make_shared<LuaReference>(L);
	popAll();

	registerCore();

	popAll();

	LuaStack S(L);
	api::Enums enums;

	S.push(env->game());
	lua_setglobal(L, "GAME");

	S.push(env->services());
	lua_setglobal(L, "LIBRARY");

	S.push(enums);
	lua_setglobal(L, "ENUM");

	popAll();
}

LuaContext::~LuaContext()
{
	modules.reset();
	scriptClosure.reset();
	lua_close(L);
}

void LuaContext::cleanupGlobals()
{
	LuaStack S(L);
	S.clear();
	S.pushNil();
	lua_setglobal(L, "collectgarbage");

	S.pushNil();
	lua_setglobal(L, "dofile");

	S.pushNil();
	lua_setglobal(L, "load");

	S.pushNil();
	lua_setglobal(L, "loadfile");

	S.pushNil();
	lua_setglobal(L, "loadstring");

	lua_pushcfunction(L, luaPrint);
	lua_setglobal(L, "print");

	S.clear();

	lua_getglobal(L, LUA_STRLIBNAME);

	S.push("dump");
	S.pushNil();
	lua_rawset(L, -3);
	S.clear();

	lua_getglobal(L, LUA_MATHLIBNAME);

	S.push("random");
	S.pushNil();
	lua_rawset(L, -3);

	S.push("randomseed");
	S.pushNil();
	lua_rawset(L, -3);
	S.clear();
}

void LuaContext::run()
{
	std::lock_guard guard(mutex);
	int ret = luaL_loadbuffer(L, script->getSource().c_str(), script->getSource().size(), script->getJsonKey().c_str());

	if(ret)
	{
		logScript->error("Script '%s' failed to load, error: %s", script->getJsonKey(), toStringRaw(-1));
		popAll();
		return;
	}

	scriptClosure = std::make_shared<LuaReference>(L);
	popAll();
	scriptClosure->push();

	ret = lua_pcall(L, 0, 0, 0);

	if(ret)
	{
		logScript->error("Script '%s' failed to run, error: '%s'", script->getJsonKey(), toStringRaw(-1));
		popAll();
	}
}

int LuaContext::errorRetVoid(const std::string & message)
{
	logGlobal->error(message);
	popAll();
	return 0;
}

JsonNode LuaContext::callGlobal(const std::string & name, const JsonNode & parameters)
{
	std::lock_guard guard(mutex);
	LuaStack S(L);

	lua_getglobal(L, name.c_str());

	if(!S.isFunction(-1))
	{
		boost::format fmt("%s is not a function");
		fmt % name;

		logGlobal->error(fmt.str());

		S.clear();

		return JsonNode();
	}

	int argc = parameters.Vector().size();

	for(int idx = 0; idx < argc; idx++)
		S.push(parameters.Vector()[idx]);

	if(lua_pcall(L, argc, 1, 0))
	{
		std::string error = lua_tostring(L, -1);
		logGlobal->error("Lua function %s failed with message: %s", name, error);
		S.clear();
		return JsonNode();
	}

	JsonNode ret;

	pop(ret);
	S.balance();

	return ret;
}

void LuaContext::getGlobal(const std::string & name, int & value)
{
	LuaStack S(L);

	lua_getglobal(L, name.c_str());

	lua_Integer temp;
	if(S.tryGetInteger(-1, temp))
		value = static_cast<int>(temp);
	else
		value = 0;
	S.balance();
}

void LuaContext::getGlobal(const std::string & name, std::string & value)
{
	LuaStack S(L);

	lua_getglobal(L, name.c_str());

	if(!S.tryGet(-1, value))
		value.clear();

	S.balance();
}

void LuaContext::getGlobal(const std::string & name, double & value)
{
	LuaStack S(L);

	lua_getglobal(L, name.c_str());

	if(!S.tryGet(-1, value))
		value = 0.0;

	S.balance();
}

void LuaContext::getGlobal(const std::string & name, JsonNode & value)
{
	LuaStack S(L);

	lua_getglobal(L, name.c_str());

	pop(value);

	S.balance();
}

void LuaContext::setGlobal(const std::string & name, int value)
{
	lua_pushinteger(L, static_cast<lua_Integer>(value));
	lua_setglobal(L, name.c_str());
}

void LuaContext::setGlobal(const std::string & name, const std::string & value)
{
	lua_pushlstring(L, value.c_str(), value.size());
	lua_setglobal(L, name.c_str());
}

void LuaContext::setGlobal(const std::string & name, double value)
{
	lua_pushnumber(L, value);
	lua_setglobal(L, name.c_str());
}

void LuaContext::setGlobal(const std::string & name, const JsonNode & value)
{
	LuaStack S(L);
	S.push(value);
	lua_setglobal(L, name.c_str());
	S.balance();
}

void LuaContext::pop(JsonNode & value)
{
	auto type = lua_type(L, -1);

	switch(type)
	{
	case LUA_TNUMBER:
		value.Float() = lua_tonumber(L, -1);
		break;
	case LUA_TBOOLEAN:
		value.Bool() = (lua_toboolean(L, -1) != 0);
		break;
	case LUA_TSTRING:
		value.String() = toStringRaw(-1);
		break;
	case LUA_TTABLE:
		{
			JsonNode asVector;
			JsonNode asStruct;

			lua_pushnil(L);  /* first key */

			while(lua_next(L, -2) != 0)
			{
				/* 'key' (at index -2) and 'value' (at index -1) */

				JsonNode fieldValue;
				pop(fieldValue);

				if(lua_type(L, -1) == LUA_TNUMBER)
				{
					auto key = lua_tointeger(L, -1);

					if(key > 0)
					{
						if(asVector.Vector().size() < key)
							asVector.Vector().resize(key);
						--key;
						asVector.Vector().at(key) = fieldValue;
					}
				}
				else if(lua_isstring(L, -1))
				{
					auto key = toStringRaw(-1);
					asStruct[key] = fieldValue;
				}
			}

			if(!asVector.Vector().empty())
			{
				std::swap(value, asVector);
			}
			else
			{
				std::swap(value, asStruct);
			}
		}
		break;
	default:
		value.clear();
		break;
	}

	lua_pop(L, 1);
}

void LuaContext::push(const std::string & value)
{
	lua_pushlstring(L, value.c_str(), value.size());
}

void LuaContext::push(lua_CFunction f, void * opaque)
{
	lua_pushlightuserdata(L, opaque);
	lua_pushcclosure(L, f, 1);
}

void LuaContext::popAll()
{
	lua_settop(L, 0);
}

std::string LuaContext::toStringRaw(int index)
{
	size_t len = 0;
	const auto *raw = lua_tolstring(L, index, &len);
	return std::string(raw, len);
}

void LuaContext::registerCore()
{
	push(&LuaContext::require, this);
	lua_setglobal(L, "require");

	popAll();//just in case

	for(const auto & registar : api::Registry::get()->getCoreData())
	{
		registar.second->pushMetatable(L); //table

		modules->push(); //table modules
		push(registar.first); //table modules name
		lua_pushvalue(L, -3); //table modules name table
		lua_rawset(L, -3);

		popAll();
	}
}

int LuaContext::require(lua_State * L)
{
	auto * self = static_cast<LuaContext *>(lua_touserdata(L, lua_upvalueindex(1)));

	if(!self)
	{
		lua_pushstring(L, "internal error");
		lua_error(L);
		return 0;
	}

	return self->loadModule();
}

int LuaContext::loadModule()
{
	int argc = lua_gettop(L);

	if(argc < 1)
		return errorRetVoid("Module name required");

	//if module is loaded already, assume that module name is valid
	modules->push();
	lua_pushvalue(L, -2);
	lua_rawget(L, -2);

	if(lua_istable(L, -1))
	{
		lua_replace(L, 1);
		lua_settop(L, 1);
		return 1;
	}

	//continue with more checks
	if(!lua_isstring(L, 1))
		return errorRetVoid("Module name must be string");

	std::string resourceName = toStringRaw(1);

	if(resourceName.empty())
		return errorRetVoid("Module name is empty");

	auto temp = vstd::split(resourceName, ":");

	std::string scope;
	std::string modulePath;

	if(temp.size() <= 1)
	{
		modulePath = temp.at(0);
	}
	else
	{
		scope = temp.at(0);
		modulePath = temp.at(1);
	}

	if(scope.empty())
	{
		const auto *registar = api::Registry::get()->find(modulePath);

		if(!registar)
		{
			return errorRetVoid("Module not found: "+modulePath);
		}

		registar->pushMetatable(L);
	}
	else if(scope == ModScope::scopeBuiltin())
	{

	//	boost::algorithm::replace_all(modulePath, boost::is_any_of("\\/ "), "");

		boost::algorithm::replace_all(modulePath, ".", "/");

		auto *loader = CResourceHandler::get(ModScope::scopeBuiltin());

		modulePath = "scripts/lib/" + modulePath;

		ResourcePath id(modulePath, EResType::LUA_SCRIPT);

		if(!loader->existsResource(id))
			return errorRetVoid("Module not found: "+modulePath);

		auto rawData = loader->load(id)->readAll();

		auto sourceText = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);

		int ret = luaL_loadbuffer(L, sourceText.c_str(), sourceText.size(), modulePath.c_str());

		if(ret)
			return errorRetVoid(toStringRaw(-1));

		ret = lua_pcall(L, 0, 1, 0);

		if(ret)
		{
			logGlobal->error("Module '%s' failed to run, error: %s", modulePath, toStringRaw(-1));
			popAll();
			return 0;
		}
	}
	else
	{
		//todo: also allow loading scripts from same scope
		return errorRetVoid("No access to scope "+scope);
	}

	modules->push(); //name table modules
	lua_pushvalue(L, 1);//name table modules name

	if(!lua_isstring(L, -1))
		return errorRetVoid("Module name corrupted");

	lua_pushvalue(L, -3);//name table modules name table
	lua_rawset(L, -3);//name table modules
	lua_pop(L, 1);//name table

	lua_replace(L, 1);//table table
	lua_settop(L, 1);//table
	return 1;
}

}

VCMI_LIB_NAMESPACE_END
