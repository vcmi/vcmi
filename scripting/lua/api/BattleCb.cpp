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

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

#include "../../../lib/GameConstants.h"

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(BattleCbProxy);

const std::vector<BattleCbProxy::RegType> BattleCbProxy::REGISTER =
{
	{
		"getBattlefieldType",
		&BattleCbProxy::getBattlefieldType
	},
	{
		"getNextUnitId",
		LuaCallWrapper<const BattleCb>::createFunctor(&BattleCb::battleNextUnitId)
	},
	{
		"getTacticDistance",
		LuaCallWrapper<const BattleCb>::createFunctor(&BattleCb::battleTacticDist)
	},
	{
		"getTerrainType",
		&BattleCbProxy::getTerrainType
	},
	{
		"getUnitById",
		LuaCallWrapper<const BattleCb>::createFunctor(&BattleCb::battleGetUnitByID)
	},
	{
		"getUnitByPos",
		&BattleCbProxy::getUnitByPos
	},
	{
		"isFinished",
		LuaCallWrapper<const BattleCb>::createFunctor(&BattleCb::battleIsFinished)
	}
};

int BattleCbProxy::getBattlefieldType(lua_State * L, const BattleCb * object)
{
	LuaStack S(L);
	auto ret = object->battleGetBattlefieldType();
	S.push(static_cast<si32>(ret.num));
	return 1;
}

int BattleCbProxy::getTerrainType(lua_State * L, const BattleCb * object)
{
	LuaStack S(L);
	auto ret = object->battleTerrainType();
	S.push(static_cast<si32>(ret.num));
	return 1;
}

int BattleCbProxy::getUnitByPos(lua_State * L, const BattleCb * object)
{
	LuaStack S(L);

	BattleHex hex;

	if(!S.tryGet(1, hex.hex))
		return S.retNil();

	bool onlyAlive;

	if(!S.tryGet(2, onlyAlive))
		onlyAlive = true;//same as default value in battleGetUnitByPos

	S.push(object->battleGetUnitByPos(hex, onlyAlive));
	return 1;
}

}
}
