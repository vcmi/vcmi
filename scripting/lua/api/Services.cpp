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


namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(ServicesProxy, "Services");

const std::vector<ServicesProxy::RegType> ServicesProxy::REGISTER =
{
	{"artifacts", LuaCallWrapper<const Services>::createFunctor(&Services::artifacts)},
	{"creatures", LuaCallWrapper<const Services>::createFunctor(&Services::creatures)},
	{"factions", LuaCallWrapper<const Services>::createFunctor(&Services::factions)},
	{"heroClasses", LuaCallWrapper<const Services>::createFunctor(&Services::heroClasses)},
	{"heroTypes", LuaCallWrapper<const Services>::createFunctor(&Services::heroTypes)},
	{"spells", LuaCallWrapper<const Services>::createFunctor(&Services::spells)},
	{"skills", LuaCallWrapper<const Services>::createFunctor(&Services::skills)},
};

const std::vector<ServicesProxy::CustomRegType> ServicesProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(ArtifactServiceProxy, "Artifacts");

const std::vector<ArtifactServiceProxy::RegType> ArtifactServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<ArtifactID, Artifact>>::createFunctor(&ArtifactService::getByIndex)}
};

const std::vector<ArtifactServiceProxy::CustomRegType> ArtifactServiceProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(CreatureServiceProxy, "Creatures");

const std::vector<CreatureServiceProxy::RegType> CreatureServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<CreatureID, Creature>>::createFunctor(&CreatureService::getByIndex)}
};

const std::vector<CreatureServiceProxy::CustomRegType> CreatureServiceProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(FactionServiceProxy, "Factions");

const std::vector<FactionServiceProxy::RegType> FactionServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<FactionID, Faction>>::createFunctor(&FactionService::getByIndex)}
};

const std::vector<FactionServiceProxy::CustomRegType> FactionServiceProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(HeroClassServiceProxy, "HeroClasses");

const std::vector<HeroClassServiceProxy::RegType> HeroClassServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<HeroClassID, HeroClass>>::createFunctor(&HeroClassService::getByIndex)}
};

const std::vector<HeroClassServiceProxy::CustomRegType> HeroClassServiceProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(HeroTypeServiceProxy, "HeroTypes");

const std::vector<HeroTypeServiceProxy::RegType> HeroTypeServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<HeroTypeID, HeroType>>::createFunctor(&HeroTypeService::getByIndex)}
};

const std::vector<HeroTypeServiceProxy::CustomRegType> HeroTypeServiceProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(SkillServiceProxy, "Skills");

const std::vector<SkillServiceProxy::RegType> SkillServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<SecondarySkill, Skill>>::createFunctor(&SkillService::getByIndex)}
};

const std::vector<SkillServiceProxy::CustomRegType> SkillServiceProxy::REGISTER_CUSTOM =
{

};

VCMI_REGISTER_CORE_SCRIPT_API(SpellServiceProxy, "Spells");

const std::vector<SpellServiceProxy::RegType> SpellServiceProxy::REGISTER =
{
	{"getByIndex", LuaCallWrapper<const EntityServiceT<SpellID, spells::Spell>>::createFunctor(&spells::Service::getByIndex)}
};

const std::vector<SpellServiceProxy::CustomRegType> SpellServiceProxy::REGISTER_CUSTOM =
{

};

}
}
