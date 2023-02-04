/*
 * api/netpacks/BattleUnitsChanged.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleUnitsChanged.h"

#include "../../LuaStack.h"

#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(BattleUnitsChangedProxy, "netpacks.BattleUnitsChanged");

const std::vector<BattleUnitsChangedProxy::CustomRegType> BattleUnitsChangedProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true},
	{"add", &BattleUnitsChangedProxy::add, false},
	{"update", &BattleUnitsChangedProxy::update, false},
	{"resetState", &BattleUnitsChangedProxy::resetState, false},
	{"remove", &BattleUnitsChangedProxy::remove, false},
	{"toNetpackLight", &PackForClientProxy<BattleUnitsChangedProxy>::toNetpackLight, false}
};

int BattleUnitsChangedProxy::add(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleUnitsChanged> object;

	if(!S.tryGet(1, object))
		return S.retVoid();

	uint32_t id = 0;

	if(!S.tryGet(2, id))
		return S.retVoid();

	UnitChanges changes(id, BattleChanges::EOperation::ADD);

	if(!S.tryGet(3, changes.data))
		return S.retVoid();

	if(!S.tryGet(4, changes.healthDelta))
		changes.healthDelta = 0;

	object->changedStacks.push_back(changes);

	return S.retVoid();
}

int BattleUnitsChangedProxy::update(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleUnitsChanged> object;

	if(!S.tryGet(1, object))
		return S.retVoid();

	uint32_t id = 0;

	if(!S.tryGet(2, id))
		return S.retVoid();

	UnitChanges changes(id, BattleChanges::EOperation::UPDATE);

	if(!S.tryGet(3, changes.data))
		return S.retVoid();

	if(!S.tryGet(4, changes.healthDelta))
		changes.healthDelta = 0;

	object->changedStacks.push_back(changes);

	return S.retVoid();
}

int BattleUnitsChangedProxy::resetState(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleUnitsChanged> object;

	if(!S.tryGet(1, object))
		return S.retVoid();
	//todo
	return S.retVoid();
}

int BattleUnitsChangedProxy::remove(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<BattleUnitsChanged> object;

	if(!S.tryGet(1, object))
		return S.retVoid();
	//todo
	return S.retVoid();
}


}
}
}

VCMI_LIB_NAMESPACE_END
