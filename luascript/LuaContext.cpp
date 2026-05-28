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

	registerPublicTypes();

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
	scriptTable.reset();
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

bool LuaContext::hasFunction(const std::string & name)
{
	std::lock_guard guard(mutex);
	if(!scriptTable)
		return false;
	LuaStack S(L);
	scriptTable->push();
	lua_getfield(L, -1, name.c_str());
	bool result = S.isFunction(-1);
	S.clear();
	return result;
}

void LuaContext::initialize()
{
	std::lock_guard guard(mutex);
	int ret = luaL_loadbuffer(L, script->getSource().c_str(), script->getSource().size(), script->getIdentifier().c_str());

	if(ret)
	{
		logScript->error("Script '%s' failed to load, error: %s", script->getIdentifier(), toStringRaw(-1));
		popAll();
		return;
	}

	scriptClosure = std::make_shared<LuaReference>(L);
	popAll();
	scriptClosure->push();

	ret = lua_pcall(L, 0, 1, 0);

	if(ret)
	{
		logScript->error("Script '%s' failed to run, error: '%s'", script->getIdentifier(), toStringRaw(-1));
		popAll();
	}

	if (!lua_istable(L, -1)) {
		logScript->error("Script '%s' did not return a table", script->getIdentifier());
		popAll();
		return;
	}

	scriptTable = std::make_shared<LuaReference>(L);
}

int LuaContext::errorRetVoid(const std::string & message)
{
	logGlobal->error(message);
	popAll();
	return 0;
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

void LuaContext::registerPublicTypes()
{
	push(&LuaContext::require, this);
	lua_setglobal(L, "require");

	popAll();//just in case

	for(const auto & registar : api::Registry::get()->getAllTypes())
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

	if(argc != 1)
		return errorRetVoid("Module name required");

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

	auto *loader = scope.empty() ? CResourceHandler::get() : CResourceHandler::get(scope);

	ScriptPath id = ScriptPath::builtinTODO(modulePath).addPrefix("SCRIPTS/");

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

	if (!lua_istable(L, -1)) {
		logScript->error("Script '%s' did not return a table", modulePath);
		popAll();
		return 0;
	}

	return 1;
}

}

VCMI_LIB_NAMESPACE_END
