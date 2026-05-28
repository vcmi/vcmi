/*
 * api/Services.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Services.h>
#include <vcmi/ArtifactService.h>
#include <vcmi/CreatureService.h>
#include <vcmi/FactionService.h>
#include <vcmi/HeroClassService.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/SkillService.h>
#include <vcmi/spells/Service.h>

#include "../../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api::library
{

class ServicesProxy : public RawPointerWrapper<const Services, ServicesProxy>
{
	static const Artifact * getArtifactByName(const Services * services, const std::string & name);
	static const Creature * getCreatureByName(const Services * services, const std::string & name);
	static const Faction * getFactionByName(const Services * services, const std::string & name);
	static const HeroClass * getHeroClassByName(const Services * services, const std::string & name);
	static const HeroType * getHeroTypeByName(const Services * services, const std::string & name);
	static const spells::Spell * getSpellByName(const Services * services, const std::string & name);
	static const Skill * getSecondarySkillByName(const Services * services, const std::string & name);

	// TODO: resources, battlefields, obstacles, engineSettings

public:
	using Wrapper = RawPointerWrapper<const Services, ServicesProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}

VCMI_LIB_NAMESPACE_END
