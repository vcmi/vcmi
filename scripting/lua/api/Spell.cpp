/*
 * api/Spell.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Spell.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
using ::spells::Spell;

VCMI_REGISTER_CORE_SCRIPT_API(SpellProxy, "Spell");

//TODO:calculateDamage,forEachSchool

const std::vector<SpellProxy::CustomRegType> SpellProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Spell, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Spell, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Spell, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Spell, decltype(&Entity::getName), &Entity::getName>::invoke, false},

	{"isAdventure", LuaMethodWrapper<Spell, decltype(&Spell::isAdventure), &Spell::isAdventure>::invoke, false},
	{"isCombat", LuaMethodWrapper<Spell, decltype(&Spell::isCombat), &Spell::isCombat>::invoke, false},
	{"isCreatureAbility", LuaMethodWrapper<Spell, decltype(&Spell::isCreatureAbility), &Spell::isCreatureAbility>::invoke, false},
	{"isPositive", LuaMethodWrapper<Spell, decltype(&Spell::isPositive), &Spell::isPositive>::invoke, false},
	{"isNegative", LuaMethodWrapper<Spell, decltype(&Spell::isNegative), &Spell::isNegative>::invoke, false},
	{"isNeutral", LuaMethodWrapper<Spell, decltype(&Spell::isNeutral), &Spell::isNeutral>::invoke, false},
	{"isDamage", LuaMethodWrapper<Spell, decltype(&Spell::isDamage), &Spell::isDamage>::invoke, false},
	{"isOffensive", LuaMethodWrapper<Spell, decltype(&Spell::isOffensive), &Spell::isOffensive>::invoke, false},
	{"isSpecial", LuaMethodWrapper<Spell, decltype(&Spell::isSpecial), &Spell::isSpecial>::invoke, false},

	{"getCost", LuaMethodWrapper<Spell, decltype(&Spell::getCost), &Spell::getCost>::invoke, false},
	{"getBasePower", LuaMethodWrapper<Spell, decltype(&Spell::getBasePower), &Spell::getBasePower>::invoke, false},
	{"getLevelPower", LuaMethodWrapper<Spell, decltype(&Spell::getLevelPower), &Spell::getLevelPower>::invoke, false},
	{"getLevelDescription", LuaMethodWrapper<Spell, decltype(&Spell::getLevelDescription), &Spell::getLevelDescription>::invoke, false},
};

}
}

VCMI_LIB_NAMESPACE_END
