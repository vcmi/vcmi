/*
 * api/Skill.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Skill.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(SkillProxy, "Skill");

const std::vector<SkillProxy::CustomRegType> SkillProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Skill, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Skill, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Skill, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Skill, decltype(&Entity::getName), &Entity::getName>::invoke, false},
};

}
}

VCMI_LIB_NAMESPACE_END
