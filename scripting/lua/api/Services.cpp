/*
 * api/Services.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Services.h"

#include <vcmi/Artifact.h>
#include <vcmi/Creature.h>
#include <vcmi/Faction.h>
#include <vcmi/HeroClass.h>
#include <vcmi/HeroType.h>
#include <vcmi/Skill.h>
#include <vcmi/spells/Spell.h>

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"


VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(ServicesProxy, "Services");

const std::vector<ServicesProxy::CustomRegType> ServicesProxy::REGISTER_CUSTOM =
{
	{"artifacts", LuaMethodWrapper<Services, decltype(&Services::artifacts), &Services::artifacts>::invoke, false},
	{"creatures", LuaMethodWrapper<Services, decltype(&Services::creatures), &Services::creatures>::invoke, false},
	{"factions", LuaMethodWrapper<Services, decltype(&Services::factions), &Services::factions>::invoke, false},
	{"heroClasses", LuaMethodWrapper<Services, decltype(&Services::heroClasses), &Services::heroClasses>::invoke, false},
	{"heroTypes", LuaMethodWrapper<Services, decltype(&Services::heroTypes), &Services::heroTypes>::invoke, false},
	{"spells", LuaMethodWrapper<Services, decltype(&Services::spells), &Services::spells>::invoke, false},
	{"skills", LuaMethodWrapper<Services, decltype(&Services::skills), &Services::skills>::invoke, false},
};

VCMI_REGISTER_CORE_SCRIPT_API(ArtifactServiceProxy, "Artifacts");

const std::vector<ArtifactServiceProxy::CustomRegType> ArtifactServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<ArtifactService, decltype(&ArtifactService::getByIndex), &ArtifactService::getByIndex>::invoke, false}
};

VCMI_REGISTER_CORE_SCRIPT_API(CreatureServiceProxy, "Creatures");

const std::vector<CreatureServiceProxy::CustomRegType> CreatureServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<CreatureService, decltype(&CreatureService::getByIndex), &CreatureService::getByIndex>::invoke, false}
};

VCMI_REGISTER_CORE_SCRIPT_API(FactionServiceProxy, "Factions");

const std::vector<FactionServiceProxy::CustomRegType> FactionServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<FactionService, decltype(&FactionService::getByIndex), &FactionService::getByIndex>::invoke, false}
};

VCMI_REGISTER_CORE_SCRIPT_API(HeroClassServiceProxy, "HeroClasses");

const std::vector<HeroClassServiceProxy::CustomRegType> HeroClassServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<HeroClassService, decltype(&HeroClassService::getByIndex), &HeroClassService::getByIndex>::invoke, false}
};

VCMI_REGISTER_CORE_SCRIPT_API(HeroTypeServiceProxy, "HeroTypes");

const std::vector<HeroTypeServiceProxy::CustomRegType> HeroTypeServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<HeroTypeService, decltype(&HeroTypeService::getByIndex), &HeroTypeService::getByIndex>::invoke, false}
};

VCMI_REGISTER_CORE_SCRIPT_API(SkillServiceProxy, "Skills");

const std::vector<SkillServiceProxy::CustomRegType> SkillServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<SkillService, decltype(&SkillService::getByIndex), &SkillService::getByIndex>::invoke, false}
};

VCMI_REGISTER_CORE_SCRIPT_API(SpellServiceProxy, "Spells");

const std::vector<SpellServiceProxy::CustomRegType> SpellServiceProxy::REGISTER_CUSTOM =
{
	{"getByIndex", LuaMethodWrapper<spells::Service, decltype(&spells::Service::getByIndex), &spells::Service::getByIndex>::invoke, false}
};

}
}

VCMI_LIB_NAMESPACE_END
