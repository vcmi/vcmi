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

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(FactionProxy, "Faction");

const std::vector<FactionProxy::RegType> FactionProxy::REGISTER = {};

const std::vector<FactionProxy::CustomRegType> FactionProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Faction, int32_t(Entity:: *)()const, &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Faction, int32_t(Entity:: *)()const, &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Faction, const std::string &(Entity:: *)()const, &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Faction, const std::string &(Entity:: *)()const, &Entity::getName>::invoke, false},
	{"hasTown", LuaMethodWrapper<Faction, bool(Faction:: *)()const, &Faction::hasTown>::invoke, false},
};

}
}
