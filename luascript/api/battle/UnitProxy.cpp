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

#include "../../../lib/GameLibrary.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api::battle
{

VCMI_REGISTER_CORE_SCRIPT_API(UnitProxy, "battle.Unit")

const std::vector<UnitProxy::CustomRegType> UnitProxy::REGISTER_CUSTOM =
{
	{"getMinDamage", LuaMethodWrapper<Unit, decltype(&ACreature::getMinDamage), &ACreature::getMinDamage>::invoke, false},
	{"getMaxDamage", LuaMethodWrapper<Unit, decltype(&ACreature::getMaxDamage), &ACreature::getMaxDamage>::invoke, false},
	{"getAttack", LuaMethodWrapper<Unit, decltype(&ACreature::getAttack), &ACreature::getAttack>::invoke, false},
	{"getDefense", LuaMethodWrapper<Unit, decltype(&ACreature::getDefense), &ACreature::getDefense>::invoke, false},
	{"isAlive", LuaMethodWrapper<Unit, decltype(&Unit::alive), &Unit::alive>::invoke, false},
	{"isClone", LuaMethodWrapper<Unit, decltype(&Unit::isClone), &Unit::isClone>::invoke, false},
	{"isSummoned", LuaMethodWrapper<Unit, decltype(&Unit::isSummoned), &Unit::isSummoned>::invoke, false},
	{"getOwner", LuaMethodWrapper<Unit, decltype(&IUnitInfo::unitOwner), &IUnitInfo::unitOwner>::invoke, false},
	{"getSlot", LuaMethodWrapper<Unit, decltype(&IUnitInfo::unitSlot), &IUnitInfo::unitSlot>::invoke, false},
	{"getPosition", LuaMethodWrapper<Unit, decltype(&Unit::getPosition), &Unit::getPosition>::invoke, false},

	{"heal", LuaFunctionWrapper<&UnitProxy::heal>::invoke, false},
	{"getCreature", LuaFunctionWrapper<&UnitProxy::getCreature>::invoke, false },
};

void UnitProxy::heal(Unit * unit, int64_t & amount, EHealLevel level, EHealPower power)
{
	unit->heal(amount, level, power);
}

const Creature * UnitProxy::getCreature(const Unit * unit)
{
	return unit->creatureId().toEntity(LIBRARY);
}

}

VCMI_LIB_NAMESPACE_END
