/*
 * api/HeroClass.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HeroClass.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(HeroClassProxy, "HeroClass");

const std::vector<HeroClassProxy::RegType> HeroClassProxy::REGISTER = {};

const std::vector<HeroClassProxy::CustomRegType> HeroClassProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<HeroClass, int32_t(Entity:: *)()const, &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<HeroClass, int32_t(Entity:: *)()const, &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<HeroClass, const std::string &(Entity:: *)()const, &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<HeroClass, const std::string &(Entity:: *)()const, &Entity::getName>::invoke, false},
};

}
}
