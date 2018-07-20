#include "StdInc.h"
#include "MapObjectsEvaluator.h"
#include "../../lib/GameConstants.h"
#include "../../lib/VCMI_Lib.h"
#include "../../lib/mapObjects/CObjectClassesHandler.h"

std::map<int, std::map<int, int>> MapObjectsEvaluator::objectDatabase = std::map<int, std::map<int, int>>();

void MapObjectsEvaluator::init()
{
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		auto newObject = std::pair<int, std::map<int, int>>(primaryID, std::map<int, int>());
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(!handler->isStaticObject() && handler->getRMGInfo().value)
			{
				newObject.second.insert(std::pair<int,int>(secondaryID, handler->getRMGInfo().value));
			}
		}
		objectDatabase.insert(newObject);
	}
}

boost::optional<int> MapObjectsEvaluator::getObjectValue(int primaryID, int secondaryID)
{
	auto object = objectDatabase.find(primaryID);
	if(object != objectDatabase.end())
	{
		auto subobjects = (*object).second;
		auto desiredObject = subobjects.find(secondaryID);
		if(desiredObject != subobjects.end())
		{
			return (*desiredObject).second;
		}
	}
	logGlobal->trace("Unknown object for AI, ID: " + std::to_string(primaryID) + " SubID: " + std::to_string(secondaryID));
	return boost::optional<int>();
}
