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
#include "QuestArtifactPlacer.h"
#include "TownPlacer.h"
#include "TerrainPainter.h"
#include "../../mapObjects/CObjectClassesHandler.h"
#include "../../mapObjects/MapObjects.h"
#include "../Functions.h"
#include "../RmgObject.h"

VCMI_LIB_NAMESPACE_BEGIN

void ObjectDistributor::process()
{
	//Do that only once
	auto lockVec = tryLockAll<ObjectDistributor>();
	if (!lockVec.empty())
	{
		for(auto & z : map.getZones())
		{
			if(auto * m = z.second->getModificator<ObjectDistributor>())
			{
				if(m->isFinished())
					return;
			}
		}
		distributeLimitedObjects();
		distributeSeerHuts();
		finished = true;
	}
}

void ObjectDistributor::init()
{
	//All of the terrain types need to be determined
	DEPENDENCY_ALL(TerrainPainter);
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
						//We already know there are some templates
						auto templates = handler->getTemplates(zone->getTerrainType());

						//FIXME: Templates empty?! Maybe zone changed terrain type over time?

						//Assume the template with fewest terrains is the most suitable
						auto temp = *boost::min_element(templates, [](std::shared_ptr<const ObjectTemplate> lhs, std::shared_ptr<const ObjectTemplate> rhs) -> bool
						{
							return lhs->getAllowedTerrains().size() < rhs->getAllowedTerrains().size();
						});

						oi.generateObject = [temp]() -> CGObjectInstance *
						{
							return VLC->objtypeh->getHandlerFor(temp->id, temp->subid)->create(temp);
						};
						
						oi.value = rmgInfo.value;
						oi.probability = rmgInfo.rarity;
						oi.templ = temp;

						//Rounding up will make sure all possible objects are exhausted
						uint32_t mapLimit = rmgInfo.mapLimit.value();
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

void ObjectDistributor::distributeSeerHuts()
{
	//TODO: Move typedef outside the class?

	//Copy by value to random shuffle
	const auto & zoneMap = map.getZones();
	RmgMap::ZoneVector zones(zoneMap.begin(), zoneMap.end());

	RandomGeneratorUtil::randomShuffle(zones, generator.rand);

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

	RandomGeneratorUtil::randomShuffle(zones, generator.rand);

	size_t allowedPrisons = generator.getPrisonsRemaning();
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