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

VCMI_LIB_NAMESPACE_BEGIN

using scripting::api::netpacks::InfoWindowProxy;
using scripting::api::RegisterAPI;

VCMI_REGISTER_SCRIPT_API(InfoWindowProxy, "netpacks.InfoWindow")

namespace scripting
{
namespace api
{
namespace netpacks
{

const std::vector<InfoWindowProxy::CustomRegType> InfoWindowProxy::REGISTER_CUSTOM =
{
	{"new", &Wrapper::constructor, true},
	{"addReplacement", &InfoWindowProxy::addReplacement, false},
	{"addText",	&InfoWindowProxy::addText, false},
	{"setPlayer", &InfoWindowProxy::setPlayer, false},
	{"toNetpackLight", &PackForClientProxy<InfoWindowProxy>::toNetpackLight, false}
};

int InfoWindowProxy::addReplacement(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<InfoWindow> object;

	if(!S.tryGet(1, object))
		return S.retVoid();

	int top = lua_gettop(L);

	if(top == 2)
	{
		if(lua_isstring(L, 2))
		{
			size_t len = 0;
			const auto *raw = lua_tolstring(L, 2, &len);
			std::string text(raw, len);

			object->text.replaceRawString(text);
		}
		else if(lua_isnumber(L, 2))
		{
			object->text.replaceNumber(lua_tointeger(L, 2));
		}
	}
	else if(top >= 3)
	{
		if(lua_isnumber(L, 2) && lua_isnumber(L, 3))
			object->text.replaceLocalString(static_cast<EMetaText>(lua_tointeger(L, 2)), lua_tointeger(L, 3));
	}

	return S.retVoid();
}

int InfoWindowProxy::addText(lua_State * L)
{
	LuaStack S(L);
	std::shared_ptr<InfoWindow> object;

	if(!S.tryGet(1, object))
		return S.retVoid();

	int top = lua_gettop(L);

	if(top == 2)
	{
		if(lua_isstring(L, 2))
		{
			size_t len = 0;
			const auto *raw = lua_tolstring(L, 2, &len);
			std::string text(raw, len);

			object->text.appendRawString(text);
		}
		else if(lua_isnumber(L, 2))
		{
			object->text.appendNumber(lua_tointeger(L, 2));
		}
	}

	if(top >= 3)
	{
		if(lua_isnumber(L, 2) && lua_isnumber(L, 3))
			object->text.appendLocalString(static_cast<EMetaText>(lua_tointeger(L, 2)), lua_tointeger(L, 3));
	}

	return S.retVoid();
}

int InfoWindowProxy::setPlayer(lua_State * L)
{
	LuaStack S(L);

	std::shared_ptr<InfoWindow> object;

	if(!S.tryGet(1, object))
		return S.retVoid();

	PlayerColor value;

	if(S.tryGet(2, value))
		object->player = value;

	return S.retVoid();
}


}
}
}

VCMI_LIB_NAMESPACE_END
