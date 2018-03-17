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

const std::vector<CreatureProxy::RegType> CreatureProxy::REGISTER =
{
	{"accessBonuses", LuaCallWrapper<const EntityWithBonuses<CreatureID>>::createFunctor(&EntityWithBonuses<CreatureID>::accessBonuses)},

	{"getCost", LuaCallWrapper<const Creature>::createFunctor(&Creature::getCost)},
	{"isDoubleWide", LuaCallWrapper<const Creature>::createFunctor(&Creature::isDoubleWide)},
};

const std::vector<CreatureProxy::CustomRegType> CreatureProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Creature, int32_t(Entity:: *)()const, &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Creature, int32_t(Entity:: *)()const, &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Creature, const std::string &(Entity:: *)()const, &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Creature, const std::string &(Entity:: *)()const, &Entity::getName>::invoke, false},
	{"accessBonuses", LuaMethodWrapper<Creature, const IBonusBearer *(EntityWithBonuses<CreatureID>:: *)()const, &EntityWithBonuses<CreatureID>::accessBonuses>::invoke, false},

	{"getMaxHealth", LuaMethodWrapper<Creature, uint32_t(Creature:: *)()const, &Creature::getMaxHealth>::invoke, false},
	{"getPluralName", LuaMethodWrapper<Creature, const std::string &(Creature:: *)()const, &Creature::getPluralName>::invoke, false},
	{"getSingularName", LuaMethodWrapper<Creature, const std::string &(Creature:: *)()const, &Creature::getSingularName>::invoke, false},

	{"getAdvMapAmountMin", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getAdvMapAmountMin>::invoke, false},
	{"getAdvMapAmountMax", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getAdvMapAmountMax>::invoke, false},
	{"getAIValue", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getAIValue>::invoke, false},
	{"getFightValue", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getFightValue>::invoke, false},
	{"getLevel", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getLevel>::invoke, false},
	{"getGrowth", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getGrowth>::invoke, false},
	{"getHorde", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getHorde>::invoke, false},
	{"getFactionIndex", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getFactionIndex>::invoke, false},

	{"getBaseAttack", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseAttack>::invoke, false},
	{"getBaseDefense", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseDefense>::invoke, false},
	{"getBaseDamageMin", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseDamageMin>::invoke, false},
	{"getBaseDamageMax", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseDamageMax>::invoke, false},
	{"getBaseHitPoints", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseHitPoints>::invoke, false},
	{"getBaseSpellPoints", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseSpellPoints>::invoke, false},
	{"getBaseSpeed", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseSpeed>::invoke, false},
	{"getBaseShots", LuaMethodWrapper<Creature, int32_t(Creature:: *)()const, &Creature::getBaseShots>::invoke, false},

};

}
}
