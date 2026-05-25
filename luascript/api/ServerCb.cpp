/*
 * ServerCb.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ServerCb.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/battle/Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(ServerCbProxy, "game.Server");

const std::vector<ServerCbProxy::CustomRegType> ServerCbProxy::REGISTER_CUSTOM =
{
		{ "createUnit", &ServerCbProxy::createUnit, false },
		{ "updateUnit", &ServerCbProxy::updateUnit, false },
		{ "healUnit", &ServerCbProxy::healUnit, false },
		{ "removeUnit", &ServerCbProxy::removeUnit, false },
};

int ServerCbProxy::commitPackage(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;

	if(!S.tryGet(1, object))
		return S.retNil();

	lua_remove(L, 1);

	if(lua_isuserdata(L, 1) != 1)
		return S.retVoid();

	lua_getfield(L, 1, "toNetpackLight");
	lua_insert(L, 1);

	int ret = lua_pcall(L, 1, 1, 0);

	if(ret != 0 || !lua_islightuserdata(L, 1))
		return S.retVoid();


	auto * pack = static_cast<CPackForClient *>(lua_touserdata(L, 1));

	object->apply(*pack);

	return S.retVoid();
}

template<typename NetPack>
int ServerCbProxy::apply(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;

	if(!S.tryGet(1, object))
		return S.retNil();

	lua_remove(L, 1);

	std::shared_ptr<NetPack> pack;

	if(!S.tryGet(1, pack))
		return S.retVoid();

	object->apply(*pack);

	return S.retVoid();
}

int ServerCbProxy::createUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	BattleUnitsChanged buc;
	UnitChanges uc;
	uc.operation = UnitChanges::EOperation::ADD;

	if (!S.tryGetAll(1, object, buc.battleID, uc.id, uc.data))
		return S.retVoid();

	buc.changedStacks.push_back(uc);

	object->apply(buc);
	return S.retVoid();
}

int ServerCbProxy::healUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	BattleID battleID;
	const battle::Unit * unit = nullptr;
	int64_t healthDelta;
	EHealPower healPower;
	EHealLevel healLevel;

	if (!S.tryGetAll(1, object, battleID, unit, healthDelta, healLevel, healPower))
		return S.retVoid();

	auto changedUnit = unit->acquire();
	changedUnit->heal(healthDelta, healLevel, healPower);

	BattleUnitsChanged buc;
	UnitChanges uc;
	buc.battleID = battleID;
	uc.operation = UnitChanges::EOperation::UPDATE;
	uc.id = unit->unitId();
	uc.data = changedUnit->save();
	uc.healthDelta = healthDelta;
	buc.changedStacks.push_back(uc);

	object->apply(buc);
	return S.retVoid();
}

int ServerCbProxy::updateUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	BattleUnitsChanged buc;
	UnitChanges uc;
	uc.operation = UnitChanges::EOperation::UPDATE;

	if (!S.tryGetAll(1, object, buc.battleID, uc.id, uc.data, uc.healthDelta))
		return S.retVoid();

	buc.changedStacks.push_back(uc);

	object->apply(buc);
	return S.retVoid();
}

int ServerCbProxy::removeUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	BattleUnitsChanged buc;
	UnitChanges uc;
	uc.operation = UnitChanges::EOperation::REMOVE;

	if (!S.tryGetAll(1, object, buc.battleID, uc.id))
		return S.retVoid();

	buc.changedStacks.push_back(uc);

	object->apply(buc);
	return S.retVoid();
}


}
}

VCMI_LIB_NAMESPACE_END
