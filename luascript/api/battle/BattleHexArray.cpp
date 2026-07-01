/*
 * BattleHexArray.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "BattleHexArray.h"

#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void BattleHexArrayProxy::registerMethods(MethodRegistrar & R)
{
	R.cfunction<&BattleHexArrayProxy::insert>("insert",
		{{"hex", "BattleHex", "Hex to insert."}}, {},
		"Adds the given hex to the list (no-op if already present).");
	R.cfunction<&BattleHexArrayProxy::erase>("erase",
		{{"hex", "BattleHex", "Hex to remove."}}, {},
		"Removes the given hex from the list (no-op if absent).");
	R.method<&BattleHexArray::contains>("contains",
		{{"hex", "Hex to test for membership."}}, {},
		"True if the list contains the given hex.");
	R.method<&BattleHexArray::size>("size", {},
		"Returns the number of unique hexes stored in the list.");
	R.function<&BattleHexArrayProxy::at>("at",
		{{"index", "1-based position of the hex to fetch."}}, {},
		"Returns the hex at the given 1-based index.");
}

int BattleHexArrayProxy::insert(lua_State * L)
{
	LuaStack S(L);
	void * raw = luaL_checkudata(L, 1, api::Registry::get()->getTypeName<BattleHexArray>());
	BattleHex hex;
	S.get(2, hex);
	S.clear();
	if(raw)
		static_cast<BattleHexArray *>(raw)->insert(hex);
	return 0;
}

int BattleHexArrayProxy::erase(lua_State * L)
{
	LuaStack S(L);
	void * raw = luaL_checkudata(L, 1, api::Registry::get()->getTypeName<BattleHexArray>());
	BattleHex hex;
	S.get(2, hex);
	S.clear();
	if(raw)
	{
		auto * hexes = static_cast<BattleHexArray *>(raw);
		if(hexes->contains(hex))
			hexes->erase(hex);
	}
	return 0;
}

BattleHex BattleHexArrayProxy::at(const BattleHexArray & hexes, int index)
{
	return hexes.at(index - 1);
}

}

VCMI_LIB_NAMESPACE_END
