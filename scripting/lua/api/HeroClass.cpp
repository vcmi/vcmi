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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(HeroClassProxy, "HeroClass");

const std::vector<HeroClassProxy::CustomRegType> HeroClassProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<HeroClass, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<HeroClass, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<HeroClass, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<HeroClass, decltype(&Entity::getName), &Entity::getName>::invoke, false},
};

}
}

VCMI_LIB_NAMESPACE_END
