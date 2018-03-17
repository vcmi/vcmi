/*
 * api/netpacks/PackForClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"

#include "../../../../lib/NetPacks.h"

namespace scripting
{
namespace api
{
namespace netpacks
{

template <typename Derived>
class PackForClientProxy
{
public:
	static int toNetpackLight(lua_State * L, typename Derived::UDataType object)
	{
		lua_settop(L, 0);
		lua_pushlightuserdata(L, static_cast<CPackForClient *>(object.get()));
		return 1;
	}
};

}
}
}

