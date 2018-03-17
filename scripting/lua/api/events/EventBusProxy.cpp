/*
 * EventBusProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "EventBusProxy.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"

#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace events
{

VCMI_REGISTER_CORE_SCRIPT_API(EventBusProxy);

const std::vector<EventBusProxy::RegType> EventBusProxy::REGISTER =
{
};

const std::vector<EventBusProxy::CustomRegType> EventBusProxy::REGISTER_CUSTOM =
{
	{
		"subscribeBefore",
		&EventBusProxy::subscribeBefore,
		false
	},
	{
		"subscribeAfter",
		&EventBusProxy::subscribeAfter,
		false
	}
};

int EventBusProxy::subscribeBefore(lua_State * L)
{
	return subscribe(L, "subscribeBefore");
}

int EventBusProxy::subscribeAfter(lua_State * L)
{
	return subscribe(L, "subscribeAfter");
}

int EventBusProxy::subscribe(lua_State * L, const std::string & method)
{
	// subscription = method(this, eventName, functor)
	LuaStack S(L);

	std::string eventName;
	if(!S.tryGet(2, eventName))
		return S.retNil();

	eventName = "events."+eventName;

	lua_getglobal(L, "require");
	S.push(eventName);

	if(lua_pcall(L, 1, 1, 0) == 0)
	{
		//top is now event class table
		lua_getfield(L, -1, method.c_str());
		S.pushByIndex(1);//this
		S.pushByIndex(3);//functor

		if(lua_pcall(L, 2, 1, 0) == 0)
		{
			if(!lua_isuserdata(L, -1))
				S.push("Invalid subscription handle");
		}
	}

	lua_replace(L, 1);
	lua_settop(L, 1);
	return 1;
}


}
}
}

