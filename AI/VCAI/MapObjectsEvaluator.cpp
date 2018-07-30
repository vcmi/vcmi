#include "StdInc.h"
#include "MapObjectsEvaluator.h"
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"

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
				if(handler->getAiValue().is_initialized())
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = handler->getAiValue().value();
				}
				else if(VLC->objtypeh->getObjGroupAiValue(primaryID).is_initialized()) //if value is not initialized - fallback to default value for this object family if it exists
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = VLC->objtypeh->getObjGroupAiValue(primaryID).value();
				}
				else
				{
					objectDatabase[CompoundMapObjectID(primaryID, secondaryID)] = 0; //some default handling when aiValue not found
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

