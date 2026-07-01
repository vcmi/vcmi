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

#include "../Registry.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"

#include "Artifact.h"
#include "Creature.h"
#include "Faction.h"
#include "HeroClass.h"
#include "HeroType.h"
#include "Skill.h"
#include "Spell.h"
#include "SpellSchool.h"

#include <vcmi/spells/SchoolService.h>


VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void ServicesProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&ServicesProxy::getArtifactByName>("getArtifactByName",
		{{"name", "JSON key of the artifact (e.g. `core:goldenBow`)."}}, {},
		"Looks up an artifact by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getCreatureByName>("getCreatureByName",
		{{"name", "JSON key of the creature (e.g. `core:pikeman`)."}}, {},
		"Looks up a creature by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getFactionByName>("getFactionByName",
		{{"name", "JSON key of the faction (e.g. `core:castle`)."}}, {},
		"Looks up a faction by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getHeroClassByName>("getHeroClassByName",
		{{"name", "JSON key of the hero class (e.g. `core:knight`)."}}, {},
		"Looks up a hero class by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getHeroTypeByName>("getHeroTypeByName",
		{{"name", "JSON key of the hero type (e.g. `core:orrin`)."}}, {},
		"Looks up a hero type by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getSpellByName>("getSpellByName",
		{{"name", "JSON key of the spell (e.g. `core:magicArrow`)."}}, {},
		"Looks up a spell by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getSecondarySkillByName>("getSecondarySkillByName",
		{{"name", "JSON key of the secondary skill (e.g. `core:wisdom`)."}}, {},
		"Looks up a secondary skill by its JSON key. Returns nil if not found.");
	R.function<&ServicesProxy::getSpellSchoolByName>("getSpellSchoolByName",
		{{"name", "JSON key of the spell school (e.g. `core:fire`)."}}, {},
		"Looks up a spell school by its JSON key. Returns nil if not found.");
}

const Artifact * ServicesProxy::getArtifactByName(const Services * services, const std::string & name)
{
	return services->artifacts()->getByName(name);
}

const Creature * ServicesProxy::getCreatureByName(const Services * services, const std::string & name)
{
	return services->creatures()->getByName(name);
}

const Faction * ServicesProxy::getFactionByName(const Services * services, const std::string & name)
{
	return services->factions()->getByName(name);
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

const spells::SpellSchoolType * ServicesProxy::getSpellSchoolByName(const Services * services, const std::string & name)
{
	return services->spellSchools()->getByName(name);
}

}

VCMI_LIB_NAMESPACE_END
