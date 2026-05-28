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

const std::vector<ServerCbProxy::CustomRegType> ServerCbProxy::REGISTER_CUSTOM =
{
	{ "createUnit", LuaCallWrapper<&ServerCbProxy::createUnit>::invoke, false },
	{ "updateUnit", LuaCallWrapper<&ServerCbProxy::updateUnit>::invoke, false },
	{ "injureUnit", LuaCallWrapper<&ServerCbProxy::injureUnit>::invoke, false },
	{ "healUnit", LuaCallWrapper<&ServerCbProxy::healUnit>::invoke, false },
	{ "removeUnit", LuaCallWrapper<&ServerCbProxy::removeUnit>::invoke, false },
};

int ServerCbProxy::createUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	BattleUnitsChanged buc;
	UnitChanges uc;
	uc.operation = UnitChanges::EOperation::ADD;

	if (!S.tryGetAll(1, object, buc.battleID, uc.id, uc.data))
		throw LuaApiException("Invalid parameters passed into createUnit!");

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
		throw LuaApiException("Invalid parameters passed into healUnit!");

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
		throw LuaApiException("Invalid parameters passed into updateUnit!");

	buc.changedStacks.push_back(uc);

	object->apply(buc);
	return S.retVoid();
}

int ServerCbProxy::injureUnit(lua_State * L)
{
	LuaStack S(L);

//	ServerCallback * object = nullptr;
//	StacksInjured si;
//	BattleStackAttacked bsa;
//
//	if (!S.tryGetAll(1, object, si.battleID, bsa.attackerID, bsa.stackAttacked, uc.id, uc.data, uc.healthDelta))
//		throw LuaApiException("Invalid parameters passed into updateUnit!");
//
//	buc.changedStacks.push_back(uc);
//
//	object->apply(buc);
	return S.retVoid();
}

int ServerCbProxy::removeUnit(lua_State * L)
{
	LuaStack S(L);

	ServerCallback * object = nullptr;
	BattleID battleID;
	const battle::Unit * unit = nullptr;

	if (!S.tryGetAll(1, object, battleID, unit))
		throw LuaApiException("Invalid parameters passed into removeUnit!");

	BattleUnitsChanged buc;
	UnitChanges uc;
	buc.battleID = battleID;
	uc.operation = UnitChanges::EOperation::REMOVE;
	uc.id = unit->unitId();
	buc.changedStacks.push_back(uc);

	object->apply(buc);
	return S.retVoid();
}


}
}

VCMI_LIB_NAMESPACE_END
