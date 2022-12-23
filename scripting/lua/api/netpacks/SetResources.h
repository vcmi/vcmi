/*
 * api/netpacks/SetResources.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "PackForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

class SetResourcesProxy : public SharedWrapper<SetResources, SetResourcesProxy>
{
public:
	using Wrapper = SharedWrapper<SetResources, SetResourcesProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int getAbs(lua_State * L);
	static int setAbs(lua_State * L);
	static int getPlayer(lua_State * L);
	static int setPlayer(lua_State * L);
	static int getAmount(lua_State * L);
	static int setAmount(lua_State * L);
	static int clear(lua_State * L);
};

}
}
}

VCMI_LIB_NAMESPACE_END
