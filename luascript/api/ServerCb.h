/*
 * ServerCb.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/ServerCallback.h>

#include "../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

class ServerCbProxy : public RawPointerWrapper<ServerCallback, ServerCbProxy>
{
public:
	using Wrapper = RawPointerWrapper<ServerCallback, ServerCbProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int createUnit(lua_State * L);
	static int updateUnit(lua_State * L);
	static int healUnit(lua_State * L);
	static int injureUnit(lua_State * L);
	static int removeUnit(lua_State * L);
};

}
}

VCMI_LIB_NAMESPACE_END
