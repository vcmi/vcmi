/*
 * UnitProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "UnitProxy.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

namespace scripting
{
namespace api
{
namespace battle
{

VCMI_REGISTER_SCRIPT_API(UnitProxy, "battle.Unit")

const std::vector<UnitProxy::CustomRegType> UnitProxy::REGISTER_CUSTOM =
{
	{"getMinDamage", LuaMethodWrapper<Unit, decltype(&IBonusBearer::getMinDamage), &IBonusBearer::getMinDamage>::invoke, false},
	{"getMaxDamage", LuaMethodWrapper<Unit, decltype(&IBonusBearer::getMaxDamage), &IBonusBearer::getMaxDamage>::invoke, false},
	{"getAttack", LuaMethodWrapper<Unit, decltype(&IBonusBearer::getAttack), &IBonusBearer::getAttack>::invoke, false},
	{"getDefense", LuaMethodWrapper<Unit, decltype(&IBonusBearer::getDefense), &IBonusBearer::getDefense>::invoke, false},
	{"isAlive", LuaMethodWrapper<Unit, decltype(&Unit::alive), &Unit::alive>::invoke, false},
	{"unitId", LuaMethodWrapper<Unit, decltype(&IUnitInfo::unitId), &IUnitInfo::unitId>::invoke, false},
};

}
}
}
