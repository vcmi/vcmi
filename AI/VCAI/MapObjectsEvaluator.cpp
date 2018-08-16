#include "StdInc.h"
#include "MapObjectsEvaluator.h"
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/CRandomGenerator.h"

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
			if(!handler->isStaticObject())
			{
				if(handler->getAiValue() != boost::none)
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = handler->getAiValue().get();
				}
				else if(VLC->objtypeh->getObjGroupAiValue(primaryID) != boost::none) //if value is not initialized - fallback to default value for this object family if it exists
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = VLC->objtypeh->getObjGroupAiValue(primaryID).get();
				}
				else //some default handling when aiValue not found
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
	if(obj->ID == Obj::CREATURE_GENERATOR1 || obj->ID == Obj::CREATURE_GENERATOR4)
	{
		auto dwelling = dynamic_cast<const CGDwelling *>(obj);
		int aiValue = 0;
		for(auto & creLevel : dwelling->creatures)
		{
			for(auto & creatureID : creLevel.second)
			{
				auto creature = VLC->creh->creatures[creatureID];
				aiValue += (creature->AIValue * creature->growth);
			}
		}
		return aiValue;
	}
	else
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

