#include "StdInc.h"
#include "MapObjectsEvaluator.h"
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CHeroHandler.h"
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
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler && !handler->isStaticObject())
			{
				if(handler->getAiValue() != boost::none)
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = handler->getAiValue().get();
				}
				else //some default handling when aiValue not found, objects that require advanced properties (unavailable from handler) get their value calculated in getObjectValue
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = 0;
				}
			}
		}
	}
}

boost::optional<int> MapObjectsEvaluator::getObjectValue(int primaryID, int secondaryID) const
{
	CompoundMapObjectID internalIdentifier = CompoundMapObjectID(primaryID, secondaryID);
	auto object = objectDatabase.find(internalIdentifier);
	if(object != objectDatabase.end())
		return object->second;

	logGlobal->trace("Unknown object for AI, ID: " + std::to_string(primaryID) + ", SubID: " + std::to_string(secondaryID));
	return boost::optional<int>();
}

boost::optional<int> MapObjectsEvaluator::getObjectValue(const CGObjectInstance * obj) const
{
	if(obj->ID == Obj::HERO)
	{
		//special case handling: in-game heroes have hero ID as object subID, but when reading configs available hero object subID's are hero classes
		auto hero = dynamic_cast<const CGHeroInstance*>(obj);
		return getObjectValue(obj->ID, hero->type->heroClass->getIndex());
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
				auto creature = VLC->creatures()->getById(creatureID);
				aiValue += (creature->getAIValue() * creature->getGrowth());
			}
		}
		return aiValue;
	}
	else if(obj->ID == Obj::ARTIFACT)
	{
		auto artifactObject = dynamic_cast<const CGArtifact *>(obj);
		switch(artifactObject->storedArtifact->artType->aClass)
		{
		case CArtifact::EartClass::ART_TREASURE:
			return 2000;
		case CArtifact::EartClass::ART_MINOR:
			return 5000;
		case CArtifact::EartClass::ART_MAJOR:
			return 10000;
		case CArtifact::EartClass::ART_RELIC:
			return 20000;
		case CArtifact::EartClass::ART_SPECIAL:
			return 20000;
		default:
			return 0; //invalid artifact class
		}
	}
	else if(obj->ID == Obj::SPELL_SCROLL)
	{
		auto scrollObject = dynamic_cast<const CGArtifact *>(obj);
		auto spell = scrollObject->storedArtifact->getGivenSpellID().toSpell();
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
			logAi->warn("AI found spell scroll with invalid spell ID: %s", scrollObject->storedArtifact->getGivenSpellID());
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

