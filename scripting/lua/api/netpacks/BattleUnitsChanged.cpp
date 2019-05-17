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

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(BattleUnitsChangedProxy, "netpacks.BattleUnitsChanged");

const std::vector<BattleUnitsChangedProxy::RegType> BattleUnitsChangedProxy::REGISTER =
{
	{
		"toNetpackLight",
		&PackForClientProxy<BattleUnitsChangedProxy>::toNetpackLight
	},
	{
		"add",
		&BattleUnitsChangedProxy::add
	},
	{
		"update",
		&BattleUnitsChangedProxy::update
	},
	{
		"resetState",
		&BattleUnitsChangedProxy::resetState
	},
	{
		"remove",
		&BattleUnitsChangedProxy::remove
	}
};

int BattleUnitsChangedProxy::add(lua_State * L, std::shared_ptr<BattleUnitsChanged> object)
{
	LuaStack S(L);
	uint32_t id;

	if(!S.tryGet(1, id))
		return S.retVoid();

	UnitChanges changes(id, BattleChanges::EOperation::ADD);

	if(!S.tryGet(2, changes.data))
		return S.retVoid();

	if(!S.tryGet(3, changes.healthDelta))
		changes.healthDelta = 0;

	object->changedStacks.push_back(changes);

	return S.retVoid();
}

int BattleUnitsChangedProxy::update(lua_State * L, std::shared_ptr<BattleUnitsChanged> object)
{
	LuaStack S(L);

	uint32_t id;

	if(!S.tryGet(1, id))
		return S.retVoid();

	UnitChanges changes(id, BattleChanges::EOperation::UPDATE);

	if(!S.tryGet(2, changes.data))
		return S.retVoid();

	if(!S.tryGet(3, changes.healthDelta))
		changes.healthDelta = 0;

	object->changedStacks.push_back(changes);

	return S.retVoid();
}

int BattleUnitsChangedProxy::resetState(lua_State * L, std::shared_ptr<BattleUnitsChanged> object)
{
	LuaStack S(L);
	return S.retVoid();
}

int BattleUnitsChangedProxy::remove(lua_State * L, std::shared_ptr<BattleUnitsChanged> object)
{
	LuaStack S(L);
	return S.retVoid();
}


}
}
}
