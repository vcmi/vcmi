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

#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN


namespace spells
{
namespace effects
{

void Effects::add(const std::string & name, std::shared_ptr<Effect> effect, const int level)
{
	effect->name = name;
	data.at(level)[name] = effect;
}

bool Effects::applicable(Problem & problem, const Mechanics * m) const
{
	//stop on first problem
	//require all not optional effects to be applicable in general
	//f.e. FireWall damage effect also need to have smart target

	bool requiredEffectNotBlocked = true;
	bool oneEffectApplicable = false;

	auto callback = [&](const Effect * e, bool & stop)
	{
		if(e->applicable(problem, m))
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

		EffectTarget target = e->transformTarget(m, aimPoint, spellTarget);

		if(e->applicable(problem, m, target))
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
	for(auto one : data.at(level))
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
			applyThis = (m->caster->getCasterUnitId() >= 0);
		else
			applyThis = !e->indirect;

		if(applyThis)
		{
			EffectTarget target = e->transformTarget(m, aimPoint, spellTarget);
			effectsToApply.push_back(std::make_pair(e, target));
		}
	};

	forEachEffect(m->getEffectLevel(), callback);

	return effectsToApply;
}

void Effects::serializeJson(const Registry * registry, JsonSerializeFormat & handler, const int level)
{
	assert(!handler.saving);

	const JsonNode & effectMap = handler.getCurrent();

	for(auto & p : effectMap.Struct())
	{
		const std::string & name = p.first;

		auto guard = handler.enterStruct(name);

		std::string type;
		handler.serializeString("type", type);

		auto effect = Effect::create(registry, type);
		if(effect)
		{
			effect->serializeJson(handler);
			add(name, effect, level);
		}
	}
}

}
}

VCMI_LIB_NAMESPACE_END
