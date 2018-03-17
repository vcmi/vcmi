/*
 * api/netpacks/SetResources.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SetResources.h"

#include "../../LuaStack.h"

#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace netpacks
{

VCMI_REGISTER_SCRIPT_API(SetResourcesProxy, "netpacks.SetResources");

const std::vector<SetResourcesProxy::RegType> SetResourcesProxy::REGISTER =
{
	{"getAbs",&SetResourcesProxy::getAbs},
	{"setAbs",&SetResourcesProxy::setAbs},
	{"getPlayer",&SetResourcesProxy::getPlayer},
	{"setPlayer",&SetResourcesProxy::setPlayer},
	{"setAmount",&SetResourcesProxy::setAmount},
	{"getAmount",&SetResourcesProxy::getAmount},
	{"clear",&SetResourcesProxy::clear},

	{
		"toNetpackLight",
		&PackForClientProxy<SetResourcesProxy>::toNetpackLight
	},
};

const std::vector<SetResourcesProxy::CustomRegType> SetResourcesProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true}
};

int SetResourcesProxy::getAbs(lua_State * L, std::shared_ptr<SetResources> object)
{
	return LuaStack::quickRetBool(L, object->abs);
}

int SetResourcesProxy::setAbs(lua_State * L, std::shared_ptr<SetResources> object)
{
	LuaStack S(L);
	bool value = false;
	if(S.tryGet(1, value))
		object->abs = value;

	return S.retVoid();
}

int SetResourcesProxy::getPlayer(lua_State * L, std::shared_ptr<SetResources> object)
{
	LuaStack S(L);
	S.clear();
	S.push(object->player);
	return 1;
}

int SetResourcesProxy::setPlayer(lua_State * L, std::shared_ptr<SetResources> object)
{
	LuaStack S(L);
	PlayerColor value;

	if(S.tryGet(1, value))
		object->player = value;

	return S.retVoid();
}

int SetResourcesProxy::getAmount(lua_State * L, std::shared_ptr<SetResources> object)
{
	LuaStack S(L);

	Res::ERes type = Res::ERes::INVALID;

	if(!S.tryGet(1, type))
		return S.retVoid();

	S.clear();

	const TQuantity amount = vstd::atOrDefault(object->res, static_cast<size_t>(type), 0);
	S.push(amount);
	return 1;
}

int SetResourcesProxy::setAmount(lua_State * L, std::shared_ptr<SetResources> object)
{
	LuaStack S(L);

	Res::ERes type = Res::ERes::INVALID;

	if(!S.tryGet(1, type))
		return S.retVoid();

	int typeIdx = static_cast<int>(type);

	if(typeIdx < 0 || typeIdx >= object->res.size())
		return S.retVoid();

	TQuantity amount = 0;

	if(!S.tryGet(2, amount))
		return S.retVoid();

	object->res.at(typeIdx) = amount;

	return S.retVoid();
}

int SetResourcesProxy::clear(lua_State * L, std::shared_ptr<SetResources> object)
{
	LuaStack S(L);
	object->res.amin(0);
	object->res.positive();
	return S.retVoid();
}


}
}
}
