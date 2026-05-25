/*
 * BattleCb.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleCb.h"
#include <vcmi/Entity.h>

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

#include "../../lib/GameConstants.h"
#include "../../lib/battle/Unit.h"
#include "../../lib/BattleFieldHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

VCMI_REGISTER_CORE_SCRIPT_API(BattleCbProxy, "game.Battle");

const std::vector<BattleCbProxy::CustomRegType> BattleCbProxy::REGISTER_CUSTOM =
{
	{ "getNextUnitId", LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleNextUnitId), &BattleCb::battleNextUnitId>::invoke, false },
	{ "getTacticDistance", LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleTacticDist), &BattleCb::battleTacticDist>::invoke, false },
	{ "getUnitById", LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleGetUnitByID), &BattleCb::battleGetUnitByID>::invoke, false },
	{ "isFinished", LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleIsFinished), &BattleCb::battleIsFinished>::invoke, false },

	{ "getAvailableHex", LuaCallWrapper<&BattleCbProxy::getAvailableHex>::invoke, false },
	{ "getBattlefieldType", LuaCallWrapper<&BattleCbProxy::getBattlefieldType>::invoke, false },
	{ "getTerrainType", LuaCallWrapper<&BattleCbProxy::getTerrainType>::invoke, false },
	{ "getUnitByPos", LuaCallWrapper<&BattleCbProxy::getUnitByPos>::invoke, false },
	{ "getAnyUnitIf", LuaCallWrapper<&BattleCbProxy::getAnyUnitIf>::invoke, false },
};

int BattleCbProxy::getBattlefieldType(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNullOrThrow(1, object);
	auto ret = object->battleGetBattlefieldType();

	return LuaStack::quickRetStr(L, ret.getInfo()->identifier);
}

int BattleCbProxy::getTerrainType(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNullOrThrow(1, object);

	return LuaStack::quickRetInt(L, object->battleTerrainType().getNum());
}

int BattleCbProxy::getAvailableHex(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNullOrThrow(1, object);

	const Creature * creature;
	BattleSide side;
	si16 hexVal = BattleHex::INVALID;

	if(!S.tryGet(2, creature) || !S.tryGet(3, side))
		return S.retNil();

	S.tryGet(4, hexVal);

	S.clear();
	BattleHex result = object->getAvailableHex(creature, side, hexVal);
	S.push(result);
	return 1;
}

int BattleCbProxy::getUnitByPos(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNullOrThrow(1, object);

	si16 hexVal;

	if(!S.tryGet(2, hexVal))
		return S.retNil();

	BattleHex hex(hexVal);

	bool onlyAlive;

	if(!S.tryGet(3, onlyAlive))
		onlyAlive = true;//same as default value in battleGetUnitByPos

	S.clear();
	S.push(object->battleGetUnitByPos(hex, onlyAlive));
	return 1;
}

int BattleCbProxy::getAnyUnitIf(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNullOrThrow(1, object);

	if(!S.isFunction(2))
		return S.retNil();

	battle::Units units = object->battleGetUnitsIf([&L](const battle::Unit * unit){
		LuaStack S2(L);
		lua_pushvalue(L, 2); // bring copy of the function to top of stack
		S2.push(unit);

		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			std::string error = lua_tostring(L, S2.absindex(-1));
			throw LuaApiException("Lua getAnyUnitIf callback failed with message: " + error);
		}

		bool result = false;
		S2.tryGet(S2.absindex(-1), result);
		S2.balance();
		return result;
	});

	S.clear();

	if (!units.empty())
		S.push(units.front());
	else
		S.pushNil();

	return 1;
}

}

VCMI_LIB_NAMESPACE_END
