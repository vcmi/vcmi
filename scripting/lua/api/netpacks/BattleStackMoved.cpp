/*
 * api/netpacks/BattleStackMoved.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleStackMoved.h"

#include "../../LuaStack.h"

#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(BattleStackMovedProxy, "netpacks.BattleStackMoved");

const std::vector<BattleStackMovedProxy::RegType> BattleStackMovedProxy::REGISTER =
{
	{
		"addTileToMove",
		&BattleStackMovedProxy::addTileToMove
	},
	{
		"setUnitId",
		&BattleStackMovedProxy::setUnitId
	},
	{
		"setDistance",
		&BattleStackMovedProxy::setDistance
	},
	{
		"setTeleporting",
		&BattleStackMovedProxy::setTeleporting
	},
	{
		"toNetpackLight",
		&PackForClientProxy<BattleStackMovedProxy>::toNetpackLight
	}
};

const std::vector<BattleStackMovedProxy::CustomRegType> BattleStackMovedProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true}
};

int BattleStackMovedProxy::addTileToMove(lua_State * L, std::shared_ptr<BattleStackMoved> object)
{
	LuaStack S(L);
	lua_Integer hex = 0;
	if(!S.tryGetInteger(1, hex))
		return S.retVoid();
	object->tilesToMove.emplace_back(hex);
	return S.retVoid();
}

int BattleStackMovedProxy::setUnitId(lua_State * L, std::shared_ptr<BattleStackMoved> object)
{
	LuaStack S(L);
	S.tryGet(1, object->stack);
	return S.retVoid();
}

int BattleStackMovedProxy::setDistance(lua_State * L, std::shared_ptr<BattleStackMoved> object)
{
	LuaStack S(L);
	S.tryGet(1, object->distance);
	return S.retVoid();
}

int BattleStackMovedProxy::setTeleporting(lua_State * L, std::shared_ptr<BattleStackMoved> object)
{
	LuaStack S(L);
	S.tryGet(1, object->teleporting);
	return S.retVoid();
}

}
}
}
