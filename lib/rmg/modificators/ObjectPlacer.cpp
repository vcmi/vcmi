/*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "ObjectPlacer.h"
#include "TownPlacer.h"
#include "ConnectionsPlacer.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "../Functions.h"

#include <vstd/RNG.h>

void ObjectPlacer::process()
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
	{
		logGlobal->error("ObjectManager doesn't exist for zone %d, skip modificator %s", zone.getId(), getName());
		return;
	}

	placeRequiredObjects(*manager);
}

void ObjectPlacer::init()
{
	DEPENDENCY(TownPlacer);
	DEPENDENCY(ConnectionsPlacer);
	POSTFUNCTION(ObjectManager);
	POSTFUNCTION(RoadPlacer);
}

bool ObjectPlacer::placeRequiredObjects(ObjectManager & manager)
{
	std::vector<std::pair<std::shared_ptr<CGObjectInstance>, ui32>> requiredObjects;

	for(const auto & [objid, info] : zone.getCustomObjects().getRequiredObjects())
	{
		for(int i = 0; i < info.first; ++i)
		{
			auto secondaryID = objid.secondaryID;
			// Unspecified subtype - pick random subobject
			if(secondaryID == -1)
			{
				auto subObjects = LIBRARY->objtypeh->knownSubObjects(objid.primaryID);
				if(subObjects.empty())
				{
					logGlobal->error("No subobjects for object type %d", objid.primaryID);
					continue;
				}
				secondaryID = *RandomGeneratorUtil::nextItem(subObjects, zone.getRand());
			}
			TObjectTypeHandler handler = LIBRARY->objtypeh->getHandlerFor(objid.primaryID, secondaryID);

			assert(handler);
			ui32 guardStrength = info.second.has_value() ? info.second.value() : handler->getRMGInfo().value;
			// Random dwelling of given level becomes a dwelling of the zone's faction (see ObjectManager::placeObject).
			// Without a guard from the template, guard it according to the value of that native dwelling
			if(!info.second.has_value() && objid.primaryID == Obj::RANDOM_DWELLING_LVL)
			{
				ui32 nativeValue = getRandomDwellingValue(map, zone.getTownType(), secondaryID); // objectLvl1 has index 0
				if(nativeValue)
					guardStrength = nativeValue;
			}
			
			auto object = handler->create(map.mapInstance->cb, nullptr);
			if (object->asOwnable() && object->tempOwner == PlayerColor::UNFLAGGABLE)
				object->setOwner(PlayerColor::NEUTRAL);		

			requiredObjects.emplace_back(object, guardStrength);
		}
	}

	//Shuffle objects to avoid patterns, but don't shuffle key objects like towns
	RandomGeneratorUtil::randomShuffle(requiredObjects, zone.getRand());
	for (const auto& obj : requiredObjects)
	{
		manager.addRequiredObject(RequiredObjectInfo(obj.first, obj.second));
	}

	return true;
}
