/*
 * Services.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class ArtifactService;
class CreatureService;
class FactionService;
class HeroClassService;
class HeroTypeService;
class SkillService;

namespace spells
{
	class Service;

	namespace effects
	{
		class Registry;
	}
}

namespace scripting
{
	class Service;
}

class DLL_LINKAGE Services
{
public:
	virtual ~Services() = default;

	virtual const ArtifactService * artifacts() const = 0;
	virtual const CreatureService * creatures() const = 0;
	virtual const FactionService * factions() const = 0;
	virtual const HeroClassService * heroClasses() const = 0;
	virtual const HeroTypeService * heroTypes() const = 0;
	virtual const scripting::Service * scripts() const = 0;
	virtual const spells::Service * spells() const = 0;
	virtual const SkillService * skills() const = 0;

	virtual const spells::effects::Registry * spellEffects() const = 0;
	virtual spells::effects::Registry * spellEffects() = 0;
	//TODO: put map object types registry access here
};
