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
#include "../../../lib/NetPacks.h"
namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(ServerCbProxy, "Server");

const std::vector<ServerCbProxy::RegType> ServerCbProxy::REGISTER =
{
	{
		"addToBattleLog",
		&ServerCbProxy::apply<BattleLogMessage>
	},
	{
		"moveUnit",
		&ServerCbProxy::apply<BattleStackMoved>
	},
	{
		"changeUnits",
		&ServerCbProxy::apply<BattleUnitsChanged>
	},
	{
		"commitPackage",
		&ServerCbProxy::commitPackage
	}
};

const std::vector<ServerCbProxy::CustomRegType> ServerCbProxy::REGISTER_CUSTOM =
{

};

int ServerCbProxy::commitPackage(lua_State * L, ServerCallback * object)
{
	if(lua_isuserdata(L, 1) != 1)
	{
		lua_settop(L, 0);
		return 0;
	}

	lua_getfield(L, 1, "toNetpackLight");
	lua_insert(L, 1);

	int ret = lua_pcall(L, 1, 1, 0);

	if(ret != 0 || !lua_islightuserdata(L, 1))
	{
		lua_settop(L, 0);
		return 0;
	}

	CPackForClient * pack = static_cast<CPackForClient *>(lua_touserdata(L, 1));

	object->apply(pack);

	lua_settop(L, 0);
	return 0;
}

template<typename NetPack>
int ServerCbProxy::apply(lua_State * L, ServerCallback * object)
{
	LuaStack S(L);

	std::shared_ptr<NetPack> pack;

	if(!S.tryGet(1, pack))
		return S.retVoid();

	object->apply(pack.get());

	return S.retVoid();
}

}
}
