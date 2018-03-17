/*
 * api/netpacks/InfoWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "InfoWindow.h"

#include "../../LuaStack.h"

#include "../Registry.h"

using scripting::api::netpacks::InfoWindowProxy;
using scripting::api::RegisterAPI;

VCMI_REGISTER_SCRIPT_API(InfoWindowProxy, "netpacks.InfoWindow")

namespace scripting
{
namespace api
{
namespace netpacks
{

const std::vector<InfoWindowProxy::RegType> InfoWindowProxy::REGISTER =
{
	{
		"addReplacement",
		&InfoWindowProxy::addReplacement
	},
	{
		"addText",
		&InfoWindowProxy::addText
	},
	{
		"setPlayer",
		&InfoWindowProxy::setPlayer
	},
	{
		"toNetpackLight",
		&PackForClientProxy<InfoWindowProxy>::toNetpackLight
	},
};

const std::vector<InfoWindowProxy::CustomRegType> InfoWindowProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true}
};

int InfoWindowProxy::addReplacement(lua_State * L, std::shared_ptr<InfoWindow> object)
{
	int top = lua_gettop(L);

	if(top == 1)
	{
		if(lua_isstring(L, 1))
		{
			size_t len = 0;
			auto raw = lua_tolstring(L, 1, &len);
			std::string text(raw, len);

			object->text.addReplacement(text);
		}
		else if(lua_isnumber(L, 1))
		{
			object->text.addReplacement(lua_tointeger(L, 1));
		}
	}
	else if(top >= 2)
	{
		if(lua_isnumber(L, 1) && lua_isnumber(L, 2))
			object->text.addReplacement(lua_tointeger(L, 1), lua_tointeger(L, 2));
	}

	lua_settop(L, 0);
	return 0;
}

int InfoWindowProxy::addText(lua_State * L, std::shared_ptr<InfoWindow> object)
{
	int top = lua_gettop(L);

	if(top == 1)
	{
		if(lua_isstring(L, 1))
		{
			size_t len = 0;
			auto raw = lua_tolstring(L, 1, &len);
			std::string text(raw, len);

			object->text << text;
		}
		else if(lua_isnumber(L, 1))
		{
			object->text << (lua_tointeger(L, 1));
		}
	}

	if(top >= 2)
	{
		if(lua_isnumber(L, 1) && lua_isnumber(L, 2))
			object->text.addTxt(lua_tointeger(L, 1), lua_tointeger(L, 2));
	}

	lua_settop(L, 0);
	return 0;
}

int InfoWindowProxy::setPlayer(lua_State * L, std::shared_ptr<InfoWindow> object)
{
	LuaStack S(L);
	PlayerColor value;

	if(S.tryGet(1, value))
		object->player = value;

	return S.retVoid();
}


}
}
}
