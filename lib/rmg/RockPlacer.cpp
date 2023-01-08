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
#include "RmgMap.h"
#include "CMapGenerator.h"
#include "Functions.h"
#include "../TerrainHandler.h"
#include "../CRandomGenerator.h"
#include "../mapping/CMapEditManager.h"

VCMI_LIB_NAMESPACE_BEGIN

void RockPlacer::process()
{
	rockTerrain = VLC->terrainTypeHandler->getById(zone.getTerrainType())->rockTerrain;
	assert(!VLC->terrainTypeHandler->getById(rockTerrain)->isPassable());
	
	accessibleArea = zone.freePaths() + zone.areaUsed();
	if(auto * m = zone.getModificator<ObjectManager>())
		accessibleArea.unite(m->getVisitableArea());
	
	//negative approach - create rock tiles first, then make sure all accessible tiles have no rock
	rockArea = zone.area().getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
	
	for(auto & z : map.getZones())
	{
		if(auto * m = z.second->getModificator<RockPlacer>())
		{
			if(m != this && !m->isFinished())
				return;
		}
	}
	
	processMap();
}

void RockPlacer::processMap()
{
	//merge all areas
	for(auto & z : map.getZones())
	{
		if(auto * m = z.second->getModificator<RockPlacer>())
		{
			map.getEditManager()->getTerrainSelection().setSelection(m->rockArea.getTilesVector());
			map.getEditManager()->drawTerrain(m->rockTerrain, &generator.rand);
		}
	}
	
	for(auto & z : map.getZones())
	{
		if(auto * m = z.second->getModificator<RockPlacer>())
		{
			//now make sure all accessible tiles have no additional rock on them
			map.getEditManager()->getTerrainSelection().setSelection(m->accessibleArea.getTilesVector());
			map.getEditManager()->drawTerrain(z.second->getTerrainType(), &generator.rand);
			m->postProcess();
		}
	}
}

void RockPlacer::postProcess()
{
	//finally mark rock tiles as occupied, spawn no obstacles there
	rockArea = zone.area().getSubarea([this](const int3 & t)
	{
		return !map.map().getTile(t).terType->isPassable();
	});
	
	zone.areaUsed().unite(rockArea);
	zone.areaPossible().subtract(rockArea);
	if(auto * m = zone.getModificator<RiverPlacer>())
		m->riverProhibit().unite(rockArea);
	if(auto * m = zone.getModificator<RoadPlacer>())
		m->areaIsolated().unite(rockArea);
}

void RockPlacer::init()
{
	POSTFUNCTION_ALL(RoadPlacer);
	DEPENDENCY(TreasurePlacer);
}

char RockPlacer::dump(const int3 & t)
{
	if(!map.map().getTile(t).terType->isPassable())
	{
		return zone.area().contains(t) ? 'R' : 'E';
	}
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
