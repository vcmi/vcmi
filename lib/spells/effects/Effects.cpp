/*
 * Effects.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Effects.h"

#include <vcmi/spells/Caster.h>

#include "../ISpellMechanics.h"

#include "../../GameLibrary.h"
#include "../../json/JsonNode.h"
#include "../../modding/IdentifierStorage.h"
#include "../../scripting/ScriptService.h"
#include "../../texts/TextIdentifier.h"


namespace spells
{
namespace effects
{

bool Effects::applicable(Problem & problem, const Mechanics * m) const
{
	//stop on first problem
	//require all not optional effects to be applicable in general
	//f.e. FireWall damage effect also need to have smart target

	bool requiredEffectNotBlocked = true;
	bool oneEffectApplicable = false;

	auto callback = [&](const Effect * e, bool & stop)
	{
		if(e->applicableGeneral(problem, m))
		{
			oneEffectApplicable = true;
		}
		else if(!e->optional)
		{
			requiredEffectNotBlocked = false;
			stop = true;
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return requiredEffectNotBlocked && oneEffectApplicable;
}

bool Effects::applicable(Problem & problem, const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	//stop on first problem
	//require all direct and not optional effects to be applicable at this aimPoint
	//f.e. FireWall do not need damage target here, only a place to put obstacle

	bool requiredEffectNotBlocked = true;
	bool oneEffectApplicable = false;

	auto callback = [&](const Effect * e, bool & stop)
	{
		if(e->indirect)
			return;

		Target target = e->transformTarget(m, aimPoint, spellTarget);

		if(e->applicableTarget(problem, m, target))
		{
			oneEffectApplicable = true;
		}
		else if(!e->optional)
		{
			requiredEffectNotBlocked = false;
			stop = true;
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return requiredEffectNotBlocked && oneEffectApplicable;
}

void Effects::forEachEffect(const int level, const std::function<void(const Effect *, bool &)> & callback) const
{
	bool stop = false;
	for(const auto& one : data.at(level))
	{
		callback(one.second.get(), stop);
		if(stop)
			return;
	}
}

Effects::EffectsToApply Effects::prepare(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	EffectsToApply effectsToApply;

	auto callback = [&](const Effect * e, bool & stop)
	{
		bool applyThis = false;

		//todo: find a better way to handle such special cases

		if(m->getSpellIndex() == SpellID::RESURRECTION && e->name == "cure")
			applyThis = (m->caster->getHeroCaster() == nullptr);
		else
			applyThis = !e->indirect;

		if(applyThis)
		{
			Target target = e->transformTarget(m, aimPoint, spellTarget);
			effectsToApply.push_back(std::make_pair(e, target));
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return effectsToApply;
}

Effects::EffectsMap Effects::loadJson(const JsonNode & effectMap, const std::string & spellScope, const std::string & spellIdentifier)
{
	EffectsMap result;

	for(const auto & [name, raw] : effectMap.Struct())
	{
		auto identifier = LIBRARY->identifiers()->getIdentifier("script", raw["type"]);

		if(!identifier.has_value())
		{
			logMod->error("Spell '%s:%s' uses unknown script '%s' as effect '%s'!", spellScope, spellIdentifier, raw["type"].String(), name);
			continue;
		}

		ScriptID effectID(*identifier);

		if(LIBRARY->scriptTypes()->getById(effectID).kind != ScriptKind::SPELL_EFFECT)
		{
			logMod->error("Spell '%s:%s' uses script '%s' as effect '%s', but that script is not a spell effect!", spellScope, spellIdentifier, raw["type"].String(), name);
			continue;
		}

		JsonNode data = raw;
		LIBRARY->scriptTypes()->prepareParameters(effectID, data, TextIdentifier("spell", spellScope, spellIdentifier, name));

		auto effect = LIBRARY->scriptTypes()->createSpellEffect(effectID);

		if(!effect)
			continue; // reported by the handler

		effect->name = name;
		effect->spellScope = spellScope;
		effect->spellIdentifier = spellIdentifier;
		effect->init(std::move(data));

		result.try_emplace(name, std::move(effect));
	}

	return result;
}

}
}
