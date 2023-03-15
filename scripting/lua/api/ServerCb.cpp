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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(ServerCbProxy, "Server");

const std::vector<ServerCbProxy::CustomRegType> ServerCbProxy::REGISTER_CUSTOM =
{
	{
		"addToBattleLog",
		&ServerCbProxy::apply<BattleLogMessage>,
		false
	},
	{
		"moveUnit",
		&ServerCbProxy::apply<BattleStackMoved>,
		false
	},
	{
		"changeUnits",
		&ServerCbProxy::apply<BattleUnitsChanged>,
		false
	},
	{
		"commitPackage",
		&ServerCbProxy::commitPackage,
		false
	}
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

	object->apply(pack);

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

	object->apply(pack.get());

	return S.retVoid();
}

}
}

VCMI_LIB_NAMESPACE_END
