/*
 * api/Faction.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Faction.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(FactionProxy, "Faction");

const std::vector<FactionProxy::CustomRegType> FactionProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Faction, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Faction, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Faction, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Faction, decltype(&Entity::getNameTranslated), &Entity::getNameTranslated>::invoke, false},
	{"hasTown", LuaMethodWrapper<Faction, decltype(&Faction::hasTown), &Faction::hasTown>::invoke, false},
};

}
}

VCMI_LIB_NAMESPACE_END
