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

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(SkillProxy, "Skill");

const std::vector<SkillProxy::RegType> SkillProxy::REGISTER = {};

const std::vector<SkillProxy::CustomRegType> SkillProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Skill, int32_t(Entity:: *)()const, &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Skill, int32_t(Entity:: *)()const, &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Skill, const std::string &(Entity:: *)()const, &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Skill, const std::string &(Entity:: *)()const, &Entity::getName>::invoke, false},
};

}
}
