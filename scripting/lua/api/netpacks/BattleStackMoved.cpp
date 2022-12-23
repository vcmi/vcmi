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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(BattleStackMovedProxy, "netpacks.BattleStackMoved");

const std::vector<BattleStackMovedProxy::CustomRegType> BattleStackMovedProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true},
	{"addTileToMove", &BattleStackMovedProxy::addTileToMove, false},
	{"setUnitId", &BattleStackMovedProxy::setUnitId, false},
	{"setDistance", &BattleStackMovedProxy::setDistance, false},
	{"setTeleporting", &BattleStackMovedProxy::setTeleporting, false},
	{"toNetpackLight", &PackForClientProxy<BattleStackMovedProxy>::toNetpackLight, false}
};

int BattleStackMovedProxy::addTileToMove(lua_State * L)
{
	LuaStack S(L);

	std::shared_ptr<BattleStackMoved> object;
	if(!S.tryGet(1, object))
		return S.retVoid();

	lua_Integer hex = 0;
	if(!S.tryGetInteger(2, hex))
		return S.retVoid();
	object->tilesToMove.emplace_back(hex);
	return S.retVoid();
}

int BattleStackMovedProxy::setUnitId(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleStackMoved> object;
	if(!S.tryGet(1, object))
		return S.retVoid();
	S.tryGet(2, object->stack);
	return S.retVoid();
}

int BattleStackMovedProxy::setDistance(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleStackMoved> object;
	if(!S.tryGet(1, object))
		return S.retVoid();
	S.tryGet(2, object->distance);
	return S.retVoid();
}

int BattleStackMovedProxy::setTeleporting(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleStackMoved> object;
	if(!S.tryGet(1, object))
		return S.retVoid();
	S.tryGet(2, object->teleporting);
	return S.retVoid();
}

}
}
}

VCMI_LIB_NAMESPACE_END
