/*
 * LuaReference.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaReference.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

LuaReference::LuaReference(lua_State * L)
	: l(L),
	doCleanup(true)
{
	key = luaL_ref(l, LUA_REGISTRYINDEX);
}

LuaReference::LuaReference(LuaReference && other)
	: l(other.l),
	key(other.key),
	doCleanup(false)
{
	std::swap(doCleanup, other.doCleanup);
}

LuaReference::~LuaReference()
{
	if(doCleanup)
		luaL_unref(l, LUA_REGISTRYINDEX, key);
}

void LuaReference::push()
{
	lua_rawgeti(l, LUA_REGISTRYINDEX, key);
}


}

VCMI_LIB_NAMESPACE_END
