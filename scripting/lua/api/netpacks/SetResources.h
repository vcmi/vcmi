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

	static const std::vector<Wrapper::RegType> REGISTER;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int getAbs(lua_State * L, std::shared_ptr<SetResources> object);
	static int setAbs(lua_State * L, std::shared_ptr<SetResources> object);
	static int getPlayer(lua_State * L, std::shared_ptr<SetResources> object);
	static int setPlayer(lua_State * L, std::shared_ptr<SetResources> object);
	static int getAmount(lua_State * L, std::shared_ptr<SetResources> object);
	static int setAmount(lua_State * L, std::shared_ptr<SetResources> object);
	static int clear(lua_State * L, std::shared_ptr<SetResources> object);
};

}
}
}
