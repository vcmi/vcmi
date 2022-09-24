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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(HeroTypeProxy, "HeroType");

const std::vector<HeroTypeProxy::CustomRegType> HeroTypeProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<HeroType, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<HeroType, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<HeroType, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<HeroType, decltype(&Entity::getName), &Entity::getName>::invoke, false},
};


}
}

VCMI_LIB_NAMESPACE_END
