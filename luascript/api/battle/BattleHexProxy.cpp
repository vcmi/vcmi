/*
 * BattleHexProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "BattleHexProxy.h"

#include "../../../lib/GameLibrary.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api::battle
{

VCMI_REGISTER_CORE_SCRIPT_API(BattleHexProxy, "battle.BattleHex")

const std::vector<BattleHexProxy::CustomRegType> BattleHexProxy::REGISTER_CUSTOM =
{
	{"isValid", LuaMethodWrapper<BattleHex, decltype(&BattleHex::isValid), &BattleHex::isValid>::invoke, false},
	{"toInteger", LuaMethodWrapper<BattleHex, decltype(&BattleHex::toInt), &BattleHex::toInt>::invoke, false},
};

}

VCMI_LIB_NAMESPACE_END
