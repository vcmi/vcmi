/*
 * api/HeroType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HeroType.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(HeroTypeProxy, "HeroType");

const std::vector<HeroTypeProxy::RegType> HeroTypeProxy::REGISTER = {};

const std::vector<HeroTypeProxy::CustomRegType> HeroTypeProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<HeroType, int32_t(Entity:: *)()const, &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<HeroType, int32_t(Entity:: *)()const, &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<HeroType, const std::string &(Entity:: *)()const, &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<HeroType, const std::string &(Entity:: *)()const, &Entity::getName>::invoke, false},
};


}
}
