/*
* ObjectDistributor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "ObjectDistributor.h"

#include "../VCMI_Lib.h"
#include "RmgMap.h"
#include "CMapGenerator.h"
#include "TreasurePlacer.h"
#include "TownPlacer.h"
#include "TerrainPainter.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "../mapObjects/MapObjects.h"
#include "Functions.h"
#include "RmgObject.h"

VCMI_LIB_NAMESPACE_BEGIN

void ObjectDistributor::process()
{
	//Firts call will add objects to ALL zones, once they were added skip it
	if (zone.getModificator<TreasurePlacer>()->getPossibleObjectsSize() == 0)
	{
		ObjectDistributor::distributeLimitedObjects();
	}
}

void ObjectDistributor::init()
{
	DEPENDENCY(TownPlacer);
	DEPENDENCY(TerrainPainter);
	POSTFUNCTION(TreasurePlacer);
}

void ObjectDistributor::distributeLimitedObjects()
{
	//FIXME: Must be called after TerrainPainter::process()

	ObjectInfo oi;
	auto zones = map.getZones();

	for (auto primaryID : VLC->objtypeh->knownObjects())
	{
		for (auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if (!handler->isStaticObject() && handler->getRMGInfo().value)
			{
				auto rmgInfo = handler->getRMGInfo();

				//Skip objects which don't have global per-map limit here 
				if (rmgInfo.mapLimit)
				{
					//Count all zones where this object can be placed
					std::vector<std::shared_ptr<Zone>> matchingZones;

					for (const auto& it : zones)
					{
						if (!handler->getTemplates(it.second->getTerrainType()).empty() &&
							rmgInfo.value <= it.second->getMaxTreasureValue())
						{
							matchingZones.push_back(it.second);
						}
					}

					size_t numZones = matchingZones.size();
					if (!numZones)
						continue;

					auto rmgInfo = handler->getRMGInfo();

					for (auto& zone : matchingZones)
					{
						auto temp = handler->getTemplates(zone->getTerrainType()).front();
						oi.generateObject = [temp]() -> CGObjectInstance *
						{
							return VLC->objtypeh->getHandlerFor(temp->id, temp->subid)->create(temp);
						};
						
						oi.value = rmgInfo.value;
						oi.probability = rmgInfo.rarity;
						oi.templ = temp;

						//Rounding up will make sure all possible objects are exhausted
						uint32_t mapLimit = rmgInfo.mapLimit.get();
						uint32_t maxPerZone = std::ceil(float(mapLimit) / numZones);

						//But not more than zone limit
						oi.maxPerZone = std::min(maxPerZone, rmgInfo.zoneLimit);
						numZones--;

						rmgInfo.setMapLimit(mapLimit - oi.maxPerZone);
						//Don't add objects with 0 count remaining
						if (oi.maxPerZone)
						{
							zone->getModificator<TreasurePlacer>()->addObjectToRandomPool(oi);
						}
					}
				}
			}
		}
	}
}

VCMI_LIB_NAMESPACE_END