/*
 * api/Creature.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Creature.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"
#include "../../../lib/HeroBonus.h"

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(CreatureProxy, "Creature");

const std::vector<CreatureProxy::CustomRegType> CreatureProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Creature, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Creature, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Creature, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Creature, decltype(&Entity::getName), &Entity::getName>::invoke, false},
	{"accessBonuses", LuaMethodWrapper<Creature, decltype(&EntityWithBonuses<CreatureID>::accessBonuses), &EntityWithBonuses<CreatureID>::accessBonuses>::invoke, false},

	{"getMaxHealth", LuaMethodWrapper<Creature,decltype(&Creature::getMaxHealth), &Creature::getMaxHealth>::invoke, false},
	{"getPluralName", LuaMethodWrapper<Creature, decltype(&Creature::getPluralName), &Creature::getPluralName>::invoke, false},
	{"getSingularName", LuaMethodWrapper<Creature, decltype(&Creature::getSingularName), &Creature::getSingularName>::invoke, false},

	{"getAdvMapAmountMin", LuaMethodWrapper<Creature, decltype(&Creature::getAdvMapAmountMin), &Creature::getAdvMapAmountMin>::invoke, false},
	{"getAdvMapAmountMax", LuaMethodWrapper<Creature, decltype(&Creature::getAdvMapAmountMax), &Creature::getAdvMapAmountMax>::invoke, false},
	{"getAIValue", LuaMethodWrapper<Creature, decltype(&Creature::getAIValue), &Creature::getAIValue>::invoke, false},
	{"getFightValue", LuaMethodWrapper<Creature, decltype(&Creature::getFightValue), &Creature::getFightValue>::invoke, false},
	{"getLevel", LuaMethodWrapper<Creature, decltype(&Creature::getLevel), &Creature::getLevel>::invoke, false},
	{"getGrowth", LuaMethodWrapper<Creature, decltype(&Creature::getGrowth), &Creature::getGrowth>::invoke, false},
	{"getHorde", LuaMethodWrapper<Creature, decltype(&Creature::getHorde), &Creature::getHorde>::invoke, false},
	{"getFactionIndex", LuaMethodWrapper<Creature, decltype(&Creature::getFactionIndex), &Creature::getFactionIndex>::invoke, false},

	{"getBaseAttack", LuaMethodWrapper<Creature, decltype(&Creature::getBaseAttack), &Creature::getBaseAttack>::invoke, false},
	{"getBaseDefense", LuaMethodWrapper<Creature, decltype(&Creature::getBaseDefense), &Creature::getBaseDefense>::invoke, false},
	{"getBaseDamageMin", LuaMethodWrapper<Creature, decltype(&Creature::getBaseDamageMin), &Creature::getBaseDamageMin>::invoke, false},
	{"getBaseDamageMax", LuaMethodWrapper<Creature, decltype(&Creature::getBaseDamageMax), &Creature::getBaseDamageMax>::invoke, false},
	{"getBaseHitPoints", LuaMethodWrapper<Creature, decltype(&Creature::getBaseHitPoints), &Creature::getBaseHitPoints>::invoke, false},
	{"getBaseSpellPoints", LuaMethodWrapper<Creature, decltype(&Creature::getBaseSpellPoints), &Creature::getBaseSpellPoints>::invoke, false},
	{"getBaseSpeed", LuaMethodWrapper<Creature, decltype(&Creature::getBaseSpeed), &Creature::getBaseSpeed>::invoke, false},
	{"getBaseShots", LuaMethodWrapper<Creature, decltype(&Creature::getBaseShots), &Creature::getBaseShots>::invoke, false},

	{"getCost", LuaMethodWrapper<Creature, decltype(&Creature::getCost), &Creature::getCost>::invoke, false},
	{"isDoubleWide", LuaMethodWrapper<Creature, decltype(&Creature::isDoubleWide), &Creature::isDoubleWide>::invoke, false},
};

}
}
