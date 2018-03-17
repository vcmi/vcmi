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

namespace scripting
{
namespace api
{
using ::spells::Spell;

VCMI_REGISTER_CORE_SCRIPT_API(SpellProxy, "Spell");

//TODO:calculateDamage,forEachSchool

const std::vector<SpellProxy::RegType> SpellProxy::REGISTER =
{
	{"getCost", LuaCallWrapper<const Spell>::createFunctor(&Spell::getCost)},
	{"getBasePower", LuaCallWrapper<const Spell>::createFunctor(&Spell::getBasePower)},
	{"getLevelPower", LuaCallWrapper<const Spell>::createFunctor(&Spell::getLevelPower)},
	{"getLevelDescription", LuaCallWrapper<const Spell>::createFunctor(&Spell::getLevelDescription)},
};

const std::vector<SpellProxy::CustomRegType> SpellProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Spell, int32_t(Entity:: *)()const, &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Spell, int32_t(Entity:: *)()const, &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Spell, const std::string &(Entity:: *)()const, &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Spell, const std::string &(Entity:: *)()const, &Entity::getName>::invoke, false},

	{"isAdventure", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isAdventure>::invoke, false},
	{"isCombat", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isCombat>::invoke, false},
	{"isCreatureAbility", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isCreatureAbility>::invoke, false},
	{"isPositive", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isPositive>::invoke, false},
	{"isNegative", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isNegative>::invoke, false},
	{"isNeutral", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isNeutral>::invoke, false},
	{"isDamage", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isDamage>::invoke, false},
	{"isOffensive", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isOffensive>::invoke, false},
	{"isSpecial", LuaMethodWrapper<Spell, bool(Spell:: *)()const, &Spell::isSpecial>::invoke, false},

};

}
}
