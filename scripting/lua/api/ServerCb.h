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

namespace scripting
{
namespace api
{

class ServerCbProxy : public OpaqueWrapper<ServerCallback, ServerCbProxy>
{
public:
	using Wrapper = OpaqueWrapper<ServerCallback, ServerCbProxy>;

	static int commitPackage(lua_State * L, ServerCallback * object);

	static const std::vector<typename Wrapper::RegType> REGISTER;

	template<typename NetPack>
	static int apply(lua_State * L, ServerCallback * object);
};

}
}
