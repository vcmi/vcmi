/*
 * BattleHexArray.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../../lib/battle/BattleHexArray.h"
#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class BattleHexArrayProxy : public CopyableWrapper<const BattleHexArray, BattleHexArrayProxy>
{
public:
	static constexpr std::string_view luaName = "BattleHexArray";
	static constexpr std::string_view luaDescription =
		"A list of BattleHex values. Used wherever the engine returns or accepts a set of tiles "
		"(reachable hexes, area-of-effect, obstacle footprint, …). Supports indexed access, "
		"insertion, and erasure; iteration order matches insertion order";

	static void registerMethods(MethodRegistrar & R);

	static int insert(lua_State * L);
	static int erase(lua_State * L);
	static BattleHex at(const BattleHexArray & hexes, int index);
};

}

VCMI_LIB_NAMESPACE_END
