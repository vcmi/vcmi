/*
 * ObstaclePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ObstaclePlacer.h"
#include "ObjectManager.h"
#include "TreasurePlacer.h"
#include "RockFiller.h"
#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "../RmgMap.h"
#include "../CMapGenerator.h"
#include "../Functions.h"
#include "../../entities/faction/CFaction.h"
#include "../../mapping/CMapEditManager.h"
#include "../../mapping/CMap.h"
#include "../../mapping/ObstacleProxy.h"
#include "../../mapObjects/CGObjectInstance.h"
#include "../../mapObjects/ObstacleSetHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void ObstaclePlacer::process()
{
	manager = zone.getModificator<ObjectManager>();
	if(!manager)
		return;

	auto faction = zone.getTownType().toFaction();

	ObstacleSetFilter filter(ObstacleSet::EObstacleType::INVALID,
							zone.getTerrainType(),
							static_cast<EMapLevel>(zone.isUnderground()),
							faction->getId(),
							faction->alignment);

	if (!prepareBiome(filter, zone.getRand()))
	{
		logGlobal->warn("Failed to prepare biome, using all possible obstacles");
		// Use all if we fail to create proper biome
		collectPossibleObstacles(zone.getTerrainType());
	}
	else
	{
		logGlobal->info("Biome prepared successfully for zone %d", zone.getId());
	}
	
	{
		auto area = zone.area();
		auto areaPossible = zone.areaPossible();
		auto areaUsed = zone.areaUsed();

		blockedArea = area->getSubarea([this](const int3& t)
		{
			return map.shouldBeBlocked(t);
		});
		blockedArea.subtract(*areaUsed);
		areaPossible->subtract(blockedArea);

		prohibitedArea = zone.freePaths() + areaUsed + manager->getVisitableArea();
		if (auto * rp = zone.getModificator<RoadPlacer>())
		{
			prohibitedArea.unite(rp->getRoads());
		}

		//Progressively block tiles, but make sure they don't seal any gap between blocks
		rmg::Area toBlock;
		do
		{
			toBlock.clear();
			for (const auto& tile : areaPossible->getTilesVector())
			{
				rmg::Area neighbors;
				rmg::Area t;
				t.add(tile);

				for (const auto& n : t.getBorderOutside())
				{
					//Area outside the map is also impassable
					if (!map.isOnMap(n) || map.shouldBeBlocked(n))
					{
						neighbors.add(n);
					}
				}
				if (neighbors.empty())
				{
					continue;
				}
				//Will only be added if it doesn't connect two disjointed blocks
				if (neighbors.connected(true)) //Do not block diagonal pass
				{
					toBlock.add(tile);
				}
			}
			areaPossible->subtract(toBlock);
			for (const auto& tile : toBlock.getTilesVector())
			{
				map.setOccupied(tile, ETileType::BLOCKED);
			}

		} while (!toBlock.empty());

		prohibitedArea.unite(areaPossible.get());
	}

	auto objs = createObstacles(zone.getRand(), map.mapInstance->cb);
	mapProxy->insertObjects(objs);
}

void ObstaclePlacer::init()
{
	DEPENDENCY(ObjectManager);
	DEPENDENCY(TreasurePlacer);
	DEPENDENCY(RoadPlacer);
	if (zone.isUnderground())
	{
		DEPENDENCY_ALL(RockFiller);
	}
	else
	{
		DEPENDENCY(WaterRoutes);
		DEPENDENCY(WaterProxy);
	}
}

bool ObstaclePlacer::isInTheMap(const int3& tile)
{
	return map.isOnMap(tile);
}

void ObstaclePlacer::placeObject(rmg::Object & object, std::set<std::shared_ptr<CGObjectInstance>> &)
{
	manager->placeObject(object, false, false);
}

std::pair<bool, bool> ObstaclePlacer::verifyCoverage(const int3 & t) const
{
	return {map.shouldBeBlocked(t), zone.areaPossible()->contains(t)};
}

void ObstaclePlacer::postProcess(const rmg::Object & object)
{
	//river processing
	riverManager = zone.getModificator<RiverPlacer>();
	if(riverManager)
	{
		const auto objTypeName = object.instances().front()->object().getTypeName();
		if(objTypeName == "mountain")
			riverManager->riverSource().unite(object.getArea());
		else if(objTypeName == "lake")
			riverManager->riverSink().unite(object.getArea());
	}
}

bool ObstaclePlacer::isProhibited(const rmg::Area & objArea) const
{
	if(prohibitedArea.overlap(objArea))
		return true;
	 
	if(!zone.area()->contains(objArea))
		return true;
	
	return false;
}

VCMI_LIB_NAMESPACE_END
