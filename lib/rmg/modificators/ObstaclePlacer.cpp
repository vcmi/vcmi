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
#include "RockPlacer.h"
#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "../RmgMap.h"
#include "../CMapGenerator.h"
#include "../../CRandomGenerator.h"
#include "../Functions.h"
#include "../../mapping/CMapEditManager.h"
#include "../../mapping/CMap.h"
#include "../../mapping/ObstacleProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

void ObstaclePlacer::process()
{
	manager = zone.getModificator<ObjectManager>();
	if(!manager)
		return;

	collectPossibleObstacles(zone.getTerrainType());
	
	blockedArea = zone.area().getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
	blockedArea.subtract(zone.areaUsed());
	zone.areaPossible().subtract(blockedArea);

	prohibitedArea = zone.freePaths() + zone.areaUsed() + manager->getVisitableArea();

	auto objs = createObstacles(zone.getRand());
	mapProxy->insertObjects(objs);
}

void ObstaclePlacer::init()
{
	DEPENDENCY(ObjectManager);
	DEPENDENCY(TreasurePlacer);
	DEPENDENCY(WaterRoutes);
	DEPENDENCY(WaterProxy);
	DEPENDENCY(RoadPlacer);
	DEPENDENCY_ALL(RockPlacer);
}

bool ObstaclePlacer::isInTheMap(const int3& tile)
{
	return map.isOnMap(tile);
}

void ObstaclePlacer::placeObject(rmg::Object & object, std::set<CGObjectInstance*> &)
{
	manager->placeObject(object, false, false);
}

std::pair<bool, bool> ObstaclePlacer::verifyCoverage(const int3 & t) const
{
	return {map.shouldBeBlocked(t), zone.areaPossible().contains(t)};
}

void ObstaclePlacer::postProcess(const rmg::Object & object)
{
	//river processing
	riverManager = zone.getModificator<RiverPlacer>();
	if(riverManager)
	{
		const auto objTypeName = object.instances().front()->object().typeName;
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
	 
	if(!zone.area().contains(objArea))
		return true;
	
	return false;
}

VCMI_LIB_NAMESPACE_END
