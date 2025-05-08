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
#include "../texts/CGeneralTextHandler.h"
#include "../json/JsonRandom.h"
#include "../GameLibrary.h"
#include "../mapObjects/CGDwelling.h"
#include "../modding/IdentifierStorage.h"

VCMI_LIB_NAMESPACE_BEGIN

bool DwellingInstanceConstructor::hasNameTextID() const
{
	return true;
}

void DwellingInstanceConstructor::initTypeData(const JsonNode & input)
{
	if (input.Struct().count("name") == 0)
		logMod->warn("Dwelling %s missing name!", getJsonKey());

	LIBRARY->generaltexth->registerString( input.getModScope(), getNameTextID(), input["name"]);

	const JsonVector & levels = input["creatures"].Vector();
	const auto totalLevels = levels.size();

	availableCreatures.resize(totalLevels);
	for(int currentLevel = 0; currentLevel < totalLevels; currentLevel++)
	{
		const JsonVector & creaturesOnLevel = levels[currentLevel].Vector();
		const auto creaturesNumber = creaturesOnLevel.size();
		availableCreatures[currentLevel].resize(creaturesNumber);

		for(int currentCreature = 0; currentCreature < creaturesNumber; currentCreature++)
		{
			LIBRARY->identifiers()->requestIdentifier("creature", creaturesOnLevel[currentCreature], [this, currentLevel, currentCreature] (si32 index)
			{
				availableCreatures.at(currentLevel).at(currentCreature) = CreatureID(index).toCreature();
			});
		}
		assert(!availableCreatures[currentLevel].empty());
	}
	guards = input["guards"];
	bannedForRandomDwelling = input["bannedForRandomDwelling"].Bool();
}

bool DwellingInstanceConstructor::isBannedForRandomDwelling() const
{
	return bannedForRandomDwelling;
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

void DwellingInstanceConstructor::randomizeObject(CGDwelling * dwelling, vstd::RNG &rng) const
{
	JsonRandom randomizer(dwelling->cb);

	dwelling->creatures.clear();
	dwelling->creatures.reserve(availableCreatures.size());

	for(const auto & entry : availableCreatures)
	{
		dwelling->creatures.resize(dwelling->creatures.size() + 1);
		for(const CCreature * cre : entry)
			dwelling->creatures.back().second.push_back(cre->getId());
	}

	bool guarded = false;

	if(guards.getType() == JsonNode::JsonType::DATA_BOOL)
	{
		//simple switch
		if(guards.Bool())
		{
			guarded = true;
		}
	}
	else if(guards.getType() == JsonNode::JsonType::DATA_VECTOR)
	{
		//custom guards (eg. Elemental Conflux)
		JsonRandom::Variables emptyVariables;
		for(auto & stack : randomizer.loadCreatures(guards, rng, emptyVariables))
		{
			dwelling->putStack(SlotID(dwelling->stacksCount()), std::make_unique<CStackInstance>(dwelling->cb, stack.getId(), stack.getCount()));
		}
	}
	else if (dwelling->ID == Obj::CREATURE_GENERATOR1 || dwelling->ID == Obj::CREATURE_GENERATOR4)
	{
		//default condition - this is dwelling with creatures of level 5 or higher
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
			dwelling->putStack(SlotID(dwelling->stacksCount()), std::make_unique<CStackInstance>(dwelling->cb, crea->getId(), crea->getGrowth() * 3));
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
