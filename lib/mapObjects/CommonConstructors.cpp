#include "StdInc.h"
#include "CommonConstructors.h"

#include "CGTownInstance.h"
#include "CGHeroInstance.h"
#include "../mapping/CMap.h"
#include "../CHeroHandler.h"
#include "../CCreatureHandler.h"

/*
 * CommonConstructors.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CObstacleConstructor::CObstacleConstructor()
{
}

bool CObstacleConstructor::isStaticObject()
{
	return true;
}

CTownInstanceConstructor::CTownInstanceConstructor():
	faction(nullptr)
{
}

void CTownInstanceConstructor::initTypeData(const JsonNode & input)
{
	VLC->modh->identifiers.requestIdentifier("faction", input["faction"], [&](si32 index)
	{
		faction = VLC->townh->factions[index];
	});

	filtersJson = input["filters"];
}

void CTownInstanceConstructor::afterLoadFinalization()
{
	assert(faction);
	for (auto entry : filtersJson.Struct())
	{
		filters[entry.first] = LogicalExpression<BuildingID>(entry.second, [this](const JsonNode & node)
		{
			return BuildingID(VLC->modh->identifiers.getIdentifier("building." + faction->identifier, node.Vector()[0]).get());
		});
	}
}

bool CTownInstanceConstructor::objectFilter(const CGObjectInstance * object, const ObjectTemplate & templ) const
{
	auto town = dynamic_cast<const CGTownInstance *>(object);

	auto buildTest = [&](const BuildingID & id)
	{
		return town->hasBuilt(id);
	};

	if (filters.count(templ.stringID))
		return filters.at(templ.stringID).test(buildTest);
	return false;
}

CGObjectInstance * CTownInstanceConstructor::create(ObjectTemplate tmpl) const
{
	CGTownInstance * obj = createTyped(tmpl);
	obj->town = faction->town;
	obj->tempOwner = PlayerColor::NEUTRAL;
	return obj;
}

void CTownInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	auto templ = getOverride(object->cb->getTile(object->pos)->terType, object);
	if (templ)
		object->appearance = templ.get();
}

CHeroInstanceConstructor::CHeroInstanceConstructor()
{

}

void CHeroInstanceConstructor::initTypeData(const JsonNode & input)
{
	VLC->modh->identifiers.requestIdentifier("heroClass", input["heroClass"],
			[&](si32 index) { heroClass = VLC->heroh->classes.heroClasses[index]; });
}

bool CHeroInstanceConstructor::objectFilter(const CGObjectInstance * object, const ObjectTemplate & templ) const
{
	return false;
}

CGObjectInstance * CHeroInstanceConstructor::create(ObjectTemplate tmpl) const
{
	CGHeroInstance * obj = createTyped(tmpl);
	obj->type = nullptr; //FIXME: set to valid value. somehow.
	return obj;
}

void CHeroInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{

}

CDwellingInstanceConstructor::CDwellingInstanceConstructor()
{

}

void CDwellingInstanceConstructor::initTypeData(const JsonNode & input)
{
	const JsonVector & levels = input["creatures"].Vector();
	availableCreatures.resize(levels.size());
	for (size_t i=0; i<levels.size(); i++)
	{
		const JsonVector & creatures = levels[i].Vector();
		availableCreatures[i].resize(creatures.size());
		for (size_t j=0; j<creatures.size(); j++)
		{
			VLC->modh->identifiers.requestIdentifier("creature", creatures[j], [=] (si32 index)
			{
				availableCreatures[i][j] = VLC->creh->creatures[index];
			});
		}
	}
	guards = input["guards"];
}

bool CDwellingInstanceConstructor::objectFilter(const CGObjectInstance *, const ObjectTemplate &) const
{
	return false;
}

CGObjectInstance * CDwellingInstanceConstructor::create(ObjectTemplate tmpl) const
{
	CGDwelling * obj = createTyped(tmpl);

	obj->creatures.resize(availableCreatures.size());
	for (auto & entry : availableCreatures)
	{
		for (const CCreature * cre : entry)
			obj->creatures.back().second.push_back(cre->idNumber);
	}
	return obj;
}

namespace
{
	si32 loadValue(const JsonNode & value, CRandomGenerator & rng, si32 defaultValue = 0)
	{
		if (value.isNull())
			return defaultValue;
		if (value.getType() == JsonNode::DATA_FLOAT)
			return value.Float();
		si32 min = value["min"].Float();
		si32 max = value["max"].Float();
		return rng.getIntRange(min, max)();
	}

	std::vector<CStackBasicDescriptor> loadCreatures(const JsonNode & value, CRandomGenerator & rng)
	{
		std::vector<CStackBasicDescriptor> ret;
		for (auto & pair : value.Struct())
		{
			CStackBasicDescriptor stack;
			stack.type = VLC->creh->creatures[VLC->modh->identifiers.getIdentifier(pair.second.meta, "creature", pair.first).get()];
			stack.count = loadValue(pair.second, rng);
			ret.push_back(stack);
		}
		return ret;
	}
}

void CDwellingInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator &rng) const
{
	CGDwelling * dwelling = dynamic_cast<CGDwelling*>(object);

	dwelling->creatures.clear();
	dwelling->creatures.resize(availableCreatures.size());

	for (auto & entry : availableCreatures)
	{
		for (const CCreature * cre : entry)
			dwelling->creatures.back().second.push_back(cre->idNumber);
	}

	for (auto & stack : loadCreatures(guards, rng))
	{
		dwelling->putStack(SlotID(dwelling->stacksCount()), new CStackInstance(stack.type->idNumber, stack.count));
	}
}

bool CDwellingInstanceConstructor::producesCreature(const CCreature * crea) const
{
	for (auto & entry : availableCreatures)
	{
		for (const CCreature * cre : entry)
			if (crea == cre)
				return true;
	}
	return false;
}
