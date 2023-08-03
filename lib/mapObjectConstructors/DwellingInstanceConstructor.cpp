/*
* DwellingInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "DwellingInstanceConstructor.h"

#include "../CCreatureHandler.h"
#include "../CGeneralTextHandler.h"
#include "../CModHandler.h"
#include "../JsonRandom.h"
#include "../VCMI_Lib.h"
#include "../mapObjects/CGDwelling.h"

VCMI_LIB_NAMESPACE_BEGIN

bool DwellingInstanceConstructor::hasNameTextID() const
{
	return true;
}

void DwellingInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (input.Struct().count("name") == 0)
		logMod->warn("Dwelling %s missing name!", getJsonKey());

	VLC->generaltexth->registerString( input.meta, getNameTextID(), input["name"].String());

	const JsonVector & levels = input["creatures"].Vector();
	const auto totalLevels = levels.size();

	availableCreatures.resize(totalLevels);
	for(auto currentLevel = 0; currentLevel < totalLevels; currentLevel++)
	{
		const JsonVector & creaturesOnLevel = levels[currentLevel].Vector();
		const auto creaturesNumber = creaturesOnLevel.size();
		availableCreatures[currentLevel].resize(creaturesNumber);

		for(auto currentCreature = 0; currentCreature < creaturesNumber; currentCreature++)
		{
			VLC->modh->identifiers.requestIdentifier("creature", creaturesOnLevel[currentCreature], [=] (si32 index)
			{
				availableCreatures[currentLevel][currentCreature] = VLC->creh->objects[index];
			});
		}
		assert(!availableCreatures[currentLevel].empty());
	}
	guards = input["guards"];
}

bool DwellingInstanceConstructor::objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return false;
}

void DwellingInstanceConstructor::initializeObject(CGDwelling * obj) const
{
	obj->creatures.resize(availableCreatures.size());
	for(const auto & entry : availableCreatures)
	{
		for(const CCreature * cre : entry)
			obj->creatures.back().second.push_back(cre->getId());
	}
}

void DwellingInstanceConstructor::randomizeObject(CGDwelling * object, CRandomGenerator &rng) const
{
	auto * dwelling = dynamic_cast<CGDwelling *>(object);

	dwelling->creatures.clear();
	dwelling->creatures.reserve(availableCreatures.size());

	for(const auto & entry : availableCreatures)
	{
		dwelling->creatures.resize(dwelling->creatures.size() + 1);
		for(const CCreature * cre : entry)
			dwelling->creatures.back().second.push_back(cre->getId());
	}

	bool guarded = false; //TODO: serialize for sanity

	if(guards.getType() == JsonNode::JsonType::DATA_BOOL) //simple switch
	{
		if(guards.Bool())
		{
			guarded = true;
		}
	}
	else if(guards.getType() == JsonNode::JsonType::DATA_VECTOR) //custom guards (eg. Elemental Conflux)
	{
		for(auto & stack : JsonRandom::loadCreatures(guards, rng))
		{
			dwelling->putStack(SlotID(dwelling->stacksCount()), new CStackInstance(stack.type->getId(), stack.count));
		}
	}
	else //default condition - creatures are of level 5 or higher
	{
		for(auto creatureEntry : availableCreatures)
		{
			if(creatureEntry.at(0)->getLevel() >= 5)
			{
				guarded = true;
				break;
			}
		}
	}

	if(guarded)
	{
		for(auto creatureEntry : availableCreatures)
		{
			const CCreature * crea = creatureEntry.at(0);
			dwelling->putStack(SlotID(dwelling->stacksCount()), new CStackInstance(crea->getId(), crea->getGrowth() * 3));
		}
	}
}

bool DwellingInstanceConstructor::producesCreature(const CCreature * crea) const
{
	for(const auto & entry : availableCreatures)
	{
		for(const CCreature * cre : entry)
			if(crea == cre)
				return true;
	}
	return false;
}

std::vector<const CCreature *> DwellingInstanceConstructor::getProducedCreatures() const
{
	std::vector<const CCreature *> creatures; //no idea why it's 2D, to be honest
	for(const auto & entry : availableCreatures)
	{
		for(const CCreature * cre : entry)
			creatures.push_back(cre);
	}
	return creatures;
}


VCMI_LIB_NAMESPACE_END
