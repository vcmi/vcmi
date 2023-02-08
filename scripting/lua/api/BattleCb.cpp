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

#include "../../../lib/GameConstants.h"
#include "../../../lib/battle/Unit.h"
#include "../../../lib/BattleFieldHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(BattleCbProxy, "Battle");

const std::vector<BattleCbProxy::CustomRegType> BattleCbProxy::REGISTER_CUSTOM =
{
	{
		"getBattlefieldType",
		&BattleCbProxy::getBattlefieldType,
		false
	},
	{
		"getNextUnitId",
		LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleNextUnitId), &BattleCb::battleNextUnitId>::invoke,
		false
	},
	{
		"getTacticDistance",
		LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleTacticDist), &BattleCb::battleTacticDist>::invoke,
		false
	},
	{
		"getTerrainType",
		&BattleCbProxy::getTerrainType,
		false
	},
	{
		"getUnitById",
		LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleGetUnitByID), &BattleCb::battleGetUnitByID>::invoke,
		false
	},
	{
		"getUnitByPos",
		&BattleCbProxy::getUnitByPos,
		false
	},
	{
		"isFinished",
		LuaMethodWrapper<BattleCb, decltype(&BattleCb::battleIsFinished), &BattleCb::battleIsFinished>::invoke,
		false
	}
};

int BattleCbProxy::getBattlefieldType(lua_State * L)
{
	LuaStack S(L);

	const BattleCb * object = nullptr;
	if(!S.tryGet(1, object))
		return S.retVoid();

	auto ret = object->battleGetBattlefieldType();

	return LuaStack::quickRetStr(L, ret.getInfo()->identifier);
}

int BattleCbProxy::getTerrainType(lua_State * L)
{
	LuaStack S(L);

	const BattleCb * object = nullptr;
	if(!S.tryGet(1, object))
		return S.retVoid();

	return LuaStack::quickRetInt(L, object->battleTerrainType().getNum());
}

int BattleCbProxy::getUnitByPos(lua_State * L)
{
	LuaStack S(L);

	const BattleCb * object = nullptr;
	if(!S.tryGet(1, object))
		return S.retVoid();

	BattleHex hex;

	if(!S.tryGet(2, hex.hex))
		return S.retNil();

	bool onlyAlive = false;

	if(!S.tryGet(3, onlyAlive))
		onlyAlive = true;//same as default value in battleGetUnitByPos

	S.clear();
	S.push(object->battleGetUnitByPos(hex, onlyAlive));
	return 1;
}

}
}

VCMI_LIB_NAMESPACE_END
