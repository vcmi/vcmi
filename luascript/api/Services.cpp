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

VCMI_REGISTER_CORE_SCRIPT_API(ServicesProxy, "library.Services");

const std::vector<ServicesProxy::CustomRegType> ServicesProxy::REGISTER_CUSTOM =
{
	{"getArtifactByName", LuaFunctionWrapper<&ServicesProxy::getArtifactByName>::invoke, false},
	{"getCreatureByName", LuaFunctionWrapper<&ServicesProxy::getCreatureByName>::invoke, false},
	{"getHeroClassByName", LuaFunctionWrapper<&ServicesProxy::getHeroClassByName>::invoke, false},
	{"getHeroTypeByName", LuaFunctionWrapper<&ServicesProxy::getHeroTypeByName>::invoke, false},
	{"getSpellByName", LuaFunctionWrapper<&ServicesProxy::getSpellByName>::invoke, false},
	{"getSecondarySkillByName", LuaFunctionWrapper<&ServicesProxy::getSecondarySkillByName>::invoke, false},
};

const Artifact * ServicesProxy::getArtifactByName(const Services * services, const std::string & name)
{
	return services->artifacts()->getByName(name);
}

const Creature * ServicesProxy::getCreatureByName(const Services * services, const std::string & name)
{
	return services->creatures()->getByName(name);
}

const HeroClass * ServicesProxy::getHeroClassByName(const Services * services, const std::string & name)
{
	return services->heroClasses()->getByName(name);
}

const HeroType * ServicesProxy::getHeroTypeByName(const Services * services, const std::string & name)
{
	return services->heroTypes()->getByName(name);
}

const spells::Spell * ServicesProxy::getSpellByName(const Services * services, const std::string & name)
{
	return services->spells()->getByName(name);
}

const Skill * ServicesProxy::getSecondarySkillByName(const Services * services, const std::string & name)
{
	return services->skills()->getByName(name);
}

}
}

VCMI_LIB_NAMESPACE_END
