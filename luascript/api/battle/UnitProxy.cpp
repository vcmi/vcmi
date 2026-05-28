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

const std::vector<UnitProxy::CustomRegType> UnitProxy::REGISTER_CUSTOM =
{
	{"getMinDamage", LuaMethodWrapper<Unit, decltype(&ACreature::getMinDamage), &ACreature::getMinDamage>::invoke, false},
	{"getMaxDamage", LuaMethodWrapper<Unit, decltype(&ACreature::getMaxDamage), &ACreature::getMaxDamage>::invoke, false},
	{"getAttack", LuaMethodWrapper<Unit, decltype(&ACreature::getAttack), &ACreature::getAttack>::invoke, false},
	{"getDefense", LuaMethodWrapper<Unit, decltype(&ACreature::getDefense), &ACreature::getDefense>::invoke, false},
	{"isAlive", LuaMethodWrapper<Unit, decltype(&Unit::alive), &Unit::alive>::invoke, false},
	{"isClone", LuaMethodWrapper<Unit, decltype(&Unit::isClone), &Unit::isClone>::invoke, false},
	{"isDead", LuaMethodWrapper<Unit, decltype(&Unit::isDead), &Unit::isDead>::invoke, false},
	{"isGhost", LuaMethodWrapper<Unit, decltype(&Unit::isGhost), &Unit::isGhost>::invoke, false},
	{"isValidTarget", LuaMethodWrapper<Unit, decltype(&Unit::isValidTarget), &Unit::isValidTarget>::invoke, false},
	{"isInvincible", LuaMethodWrapper<Unit, decltype(&Unit::isInvincible), &Unit::isInvincible>::invoke, false},
	{"hasAbsoluteImmunity", LuaFunctionWrapper<&UnitProxy::hasAbsoluteImmunity>::invoke, false},
	{"isSummoned", LuaMethodWrapper<Unit, decltype(&Unit::isSummoned), &Unit::isSummoned>::invoke, false},
	{"getOwner", LuaMethodWrapper<Unit, decltype(&IUnitInfo::unitOwner), &IUnitInfo::unitOwner>::invoke, false},
	{"getSlot", LuaMethodWrapper<Unit, decltype(&IUnitInfo::unitSlot), &IUnitInfo::unitSlot>::invoke, false},
	{"getPosition", LuaMethodWrapper<Unit, decltype(&Unit::getPosition), &Unit::getPosition>::invoke, false},
	{"getTotalHealth", LuaMethodWrapper<Unit, decltype(&Unit::getTotalHealth), &Unit::getTotalHealth>::invoke, false},
	{"coversPos", LuaMethodWrapper<Unit, decltype(&Unit::coversPos), &Unit::coversPos>::invoke, false},

	{"heal", LuaFunctionWrapper<&UnitProxy::heal>::invoke, false},
	{"getCreature", LuaFunctionWrapper<&UnitProxy::getCreature>::invoke, false},
	{"getBaseAmount", LuaFunctionWrapper<&UnitProxy::getBaseAmount>::invoke, false},
	{"getHexes", LuaFunctionWrapper<&UnitProxy::getHexes>::invoke, false},
};

void UnitProxy::heal(Unit * unit, int64_t & amount, EHealLevel level, EHealPower power)
{
	unit->heal(amount, level, power);
}

const Creature * UnitProxy::getCreature(const Unit * unit)
{
	return unit->creatureId().toEntity(LIBRARY);
}

int32_t UnitProxy::getBaseAmount(const Unit * unit)
{
	return unit->unitBaseAmount();
}

BattleHexArray UnitProxy::getHexes(const Unit * unit)
{
	return unit->getHexes();
}

bool UnitProxy::hasAbsoluteImmunity(const Unit * unit, const spells::Spell * spell)
{
	return unit->hasAbsoluteImmunity(spell->getId());
}

}

VCMI_LIB_NAMESPACE_END
