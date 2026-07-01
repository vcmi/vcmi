/*
 * BattleHex.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/BattleHexArray.h"
#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class BattleHexProxy : public CopyableWrapper<const BattleHex, BattleHexProxy>
{
public:
	static constexpr std::string_view luaName = "BattleHex";
	static constexpr std::string_view luaDescription =
		"Represents a single tile on the battlefield grid"
		"allows computing geometric relationships (distance, neighbours, line-of-sight) that don't rely on current state of a battlefield";

	static void registerMethods(MethodRegistrar & R);

	static BattleHex getClosestTile(const BattleHex & self, BattleSide side, const BattleHexArray & hexes);
};

}

VCMI_LIB_NAMESPACE_END
