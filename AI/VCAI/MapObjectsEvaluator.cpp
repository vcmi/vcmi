/*
 * MapObjectsEvaluator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MapObjectsEvaluator.h"
#include "../../lib/GameConstants.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/mapObjects/CompoundMapObjectID.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/spells/CSpellHandler.h"

MapObjectsEvaluator & MapObjectsEvaluator::getInstance()
{
	static std::unique_ptr<MapObjectsEvaluator> singletonInstance;
	if(singletonInstance == nullptr)
		singletonInstance.reset(new MapObjectsEvaluator());

	return *(singletonInstance.get());
}

MapObjectsEvaluator::MapObjectsEvaluator()
{
	for(auto primaryID : LIBRARY->objtypeh->knownObjects())
	{
		for(auto secondaryID : LIBRARY->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler && !handler->isStaticObject())
			{
				if(handler->getAiValue() != std::nullopt)
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = handler->getAiValue().value();
				}
				else //some default handling when aiValue not found, objects that require advanced properties (unavailable from handler) get their value calculated in getObjectValue
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = 0;
				}
			}
		}
	}
}

std::optional<int> MapObjectsEvaluator::getObjectValue(int primaryID, int secondaryID) const
{
	CompoundMapObjectID internalIdentifier = CompoundMapObjectID(primaryID, secondaryID);
	auto object = objectDatabase.find(internalIdentifier);
	if(object != objectDatabase.end())
		return object->second;

	logGlobal->trace("Unknown object for AI, ID: " + std::to_string(primaryID) + ", SubID: " + std::to_string(secondaryID));
	return std::optional<int>();
}

std::optional<int> MapObjectsEvaluator::getObjectValue(const CGObjectInstance * obj) const
{
	if(obj->ID == Obj::HERO)
	{
		//special case handling: in-game heroes have hero ID as object subID, but when reading configs available hero object subID's are hero classes
		auto hero = dynamic_cast<const CGHeroInstance*>(obj);
		return getObjectValue(obj->ID.getNum(), hero->getHeroClassID().getNum());
	}
	else if(obj->ID == Obj::PRISON)
	{
		//special case: in-game prison subID is captured hero ID, but config has one subID with index 0 for normal prison - use that one
		return getObjectValue(obj->ID, 0);
	}
	else if(obj->ID == Obj::CREATURE_GENERATOR1 || obj->ID == Obj::CREATURE_GENERATOR4)
	{
		auto dwelling = dynamic_cast<const CGDwelling *>(obj);
		int aiValue = 0;
		for(auto & creLevel : dwelling->creatures)
		{
			for(auto & creatureID : creLevel.second)
			{
				auto creature = LIBRARY->creatures()->getById(creatureID);
				aiValue += (creature->getAIValue() * creature->getGrowth());
			}
		}
		return aiValue;
	}
	else if(obj->ID == Obj::ARTIFACT)
	{
		auto artifactObject = dynamic_cast<const CGArtifact *>(obj);
		switch(artifactObject->getArtifactInstance()->getType()->aClass)
		{
		case EArtifactClass::ART_TREASURE:
			return 2000;
		case EArtifactClass::ART_MINOR:
			return 5000;
		case EArtifactClass::ART_MAJOR:
			return 10000;
		case EArtifactClass::ART_RELIC:
			return 20000;
		case EArtifactClass::ART_SPECIAL:
			return 20000;
		default:
			return 0; //invalid artifact class
		}
	}
	else if(obj->ID == Obj::SPELL_SCROLL)
	{
		auto scrollObject = dynamic_cast<const CGArtifact *>(obj);
		auto spell = scrollObject->getArtifactInstance()->getScrollSpellID().toSpell();
		if(spell)
		{
			switch(spell->getLevel())
			{
			case 0: return 0; //scroll with creature ability? Let's assume it is useless
			case 1: return 1000;
			case 2: return 2000;
			case 3: return 5000;
			case 4: return 10000;
			case 5: return 20000;
			default: logAi->warn("AI detected spell scroll with spell level %s", spell->getLevel());
			}
		}
		else
			logAi->warn("AI found spell scroll with invalid spell ID: %s", scrollObject->getArtifactInstance()->getScrollSpellID());
	}

	return getObjectValue(obj->ID, obj->subID);
}

void MapObjectsEvaluator::addObjectData(int primaryID, int secondaryID, int value) //by current design it updates value if already in AI database
{
	CompoundMapObjectID internalIdentifier = CompoundMapObjectID(primaryID, secondaryID);
	objectDatabase[internalIdentifier] = value;
}

void MapObjectsEvaluator::removeObjectData(int primaryID, int secondaryID)
{
	CompoundMapObjectID internalIdentifier = CompoundMapObjectID(primaryID, secondaryID);
	vstd::erase_if_present(objectDatabase, internalIdentifier);
}

