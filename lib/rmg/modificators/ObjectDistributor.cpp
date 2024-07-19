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

#include "../../VCMI_Lib.h"
#include "../RmgMap.h"
#include "../CMapGenerator.h"
#include "TreasurePlacer.h"
#include "PrisonHeroPlacer.h"
#include "QuestArtifactPlacer.h"
#include "TownPlacer.h"
#include "TerrainPainter.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/MapObjects.h"
#include "../Functions.h"
#include "../RmgObject.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void ObjectDistributor::process()
{
	distributeLimitedObjects();
	distributePrisons();
	distributeSeerHuts();
}

void ObjectDistributor::init()
{
	//All of the terrain types need to be determined
	DEPENDENCY_ALL(TerrainPainter);
	POSTFUNCTION(TreasurePlacer);
}

void ObjectDistributor::distributeLimitedObjects()
{
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

					RandomGeneratorUtil::randomShuffle(matchingZones, zone.getRand());
					for (auto& zone : matchingZones)
					{
						oi.generateObject = [cb=map.mapInstance->cb, primaryID, secondaryID]() -> CGObjectInstance *
						{
							return VLC->objtypeh->getHandlerFor(primaryID, secondaryID)->create(cb, nullptr);
						};
						
						oi.value = rmgInfo.value;
						oi.probability = rmgInfo.rarity;
						oi.setTemplates(primaryID, secondaryID, zone->getTerrainType());

						//Rounding up will make sure all possible objects are exhausted
						uint32_t mapLimit = rmgInfo.mapLimit.value();
						uint32_t maxPerZone = std::ceil(float(mapLimit) / numZones);

						//But not more than zone limit
						oi.maxPerZone = std::min(maxPerZone, rmgInfo.zoneLimit);
						numZones--;

						rmgInfo.setMapLimit(mapLimit - oi.maxPerZone);
						//Don't add objects with 0 count remaining
						if(oi.maxPerZone && !oi.templates.empty())
						{
							zone->getModificator<TreasurePlacer>()->addObjectToRandomPool(oi);
						}
					}
				}
			}
		}
	}
}

void ObjectDistributor::distributeSeerHuts()
{
	//TODO: Move typedef outside the class?

	//Copy by value to random shuffle
	const auto & zoneMap = map.getZones();
	RmgMap::ZoneVector zones(zoneMap.begin(), zoneMap.end());

	RandomGeneratorUtil::randomShuffle(zones, zone.getRand());

	const auto & possibleQuestArts = generator.getAllPossibleQuestArtifacts();
	size_t availableArts = possibleQuestArts.size();
	auto artIt = possibleQuestArts.begin();
	for (int i = zones.size() - 1; i >= 0 ; i--)
	{
		size_t localArts = std::ceil((float)availableArts / (i + 1));
		availableArts -= localArts;

		auto * qap = zones[i].second->getModificator<QuestArtifactPlacer>();
		if (qap)
		{
			for (;localArts > 0 && artIt != possibleQuestArts.end(); artIt++, localArts--)
			{
				qap->addRandomArtifact(*artIt);
			}
		}
	}
}

void ObjectDistributor::distributePrisons()
{
	//Copy by value to random shuffle
	const auto & zoneMap = map.getZones();
	RmgMap::ZoneVector zones(zoneMap.begin(), zoneMap.end());

	RandomGeneratorUtil::randomShuffle(zones, zone.getRand());

	// TODO: Some shorthand for unique Modificator
	PrisonHeroPlacer * prisonHeroPlacer = nullptr;
	for(auto & z : map.getZones())
	{
		prisonHeroPlacer = z.second->getModificator<PrisonHeroPlacer>();
		if (prisonHeroPlacer)
		{
			break;
		}
	}

	size_t allowedPrisons = prisonHeroPlacer->getPrisonsRemaining();
	for (int i = zones.size() - 1; i >= 0; i--)
	{
		auto zone = zones[i].second;
		auto * tp = zone->getModificator<TreasurePlacer>();
		if (tp)
		{
			tp->setMaxPrisons(std::ceil(float(allowedPrisons) / (i + 1)));
			allowedPrisons -= tp->getMaxPrisons();
		}
	}
}

VCMI_LIB_NAMESPACE_END
