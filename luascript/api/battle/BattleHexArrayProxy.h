/*
 * BattleHexArrayProxy.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api::battle
{

class BattleHexArrayProxy : public CopyableWrapper<const BattleHexArray, BattleHexArrayProxy>
{
public:
	using Wrapper = CopyableWrapper<const BattleHexArray, BattleHexArrayProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static void insert(BattleHexArray & hexes, BattleHex target);
	static int size(BattleHexArray & hexes);
	static BattleHex at(BattleHexArray & hexes, int index);
};

}

VCMI_LIB_NAMESPACE_END
