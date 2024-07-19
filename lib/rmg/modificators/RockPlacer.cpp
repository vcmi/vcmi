/*
 * RockPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RockPlacer.h"
#include "TreasurePlacer.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "../RmgMap.h"
#include "../CMapGenerator.h"
#include "../Functions.h"
#include "../../TerrainHandler.h"
#include "../../mapping/CMapEditManager.h"
#include "../../VCMI_Lib.h"
#include "../TileInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class TileInfo;

void RockPlacer::process()
{
	blockRock();
}

void RockPlacer::blockRock()
{
	rockTerrain = VLC->terrainTypeHandler->getById(zone.getTerrainType())->rockTerrain;
	assert(!VLC->terrainTypeHandler->getById(rockTerrain)->isPassable());

	accessibleArea = zone.freePaths() + zone.areaUsed();
	if(auto * rp = zone.getModificator<RoadPlacer>())
	{
		accessibleArea.unite(rp->getRoads());
	}
	if(auto * m = zone.getModificator<ObjectManager>())
	{
		accessibleArea.unite(m->getVisitableArea());
	}

	//negative approach - create rock tiles first, then make sure all accessible tiles have no rock
	rockArea = zone.area()->getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
}

void RockPlacer::postProcess()
{
	{
		Zone::Lock lock(zone.areaMutex);
		//Finally mark rock tiles as occupied, spawn no obstacles there
		rockArea = zone.area()->getSubarea([this](const int3 & t)
		{
			return !map.getTile(t).terType->isPassable();
		});

		// Do not place rock on roads
		if(auto * rp = zone.getModificator<RoadPlacer>())
		{
			rockArea.subtract(rp->getRoads());
		}
		
		zone.areaUsed()->unite(rockArea);
		zone.areaPossible()->subtract(rockArea);
	}

	//RecursiveLock lock(externalAccessMutex);

	if(auto * m = zone.getModificator<RiverPlacer>())
		m->riverProhibit().unite(rockArea);
	if(auto * m = zone.getModificator<RoadPlacer>())
		m->areaIsolated().unite(rockArea);
}

void RockPlacer::init()
{
	DEPENDENCY(RoadPlacer);
	for (const auto& zone : map.getZonesOnLevel(1))
	{
		auto * tp = zone.second->getModificator<TreasurePlacer>();
		if (tp)
		{
			dependency(tp);
		}
	}
}

char RockPlacer::dump(const int3 & t)
{
	if(!map.getTile(t).terType->isPassable())
	{
		return zone.area()->contains(t) ? 'R' : 'E';
	}
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
