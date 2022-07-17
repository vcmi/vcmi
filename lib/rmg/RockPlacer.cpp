/*
 * RockPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "RockPlacer.h"
#include "TreasurePlacer.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "RmgMap.h"
#include "CMapGenerator.h"
#include "../CRandomGenerator.h"
#include "../mapping/CMapEditManager.h"

void RockPlacer::process()
{
	//collect all rock terrain types
	std::vector<Terrain> rockTerrains;
	for(auto & terrain : Terrain::Manager::terrains())
		if(!terrain.isPassable())
			rockTerrains.push_back(terrain);
	auto rockTerrain = *RandomGeneratorUtil::nextItem(rockTerrains, generator.rand);
	
	auto accessibleArea = zone.freePaths() + zone.areaUsed();
	if(auto * m = zone.getModificator<ObjectManager>())
		accessibleArea.unite(m->getVisitableArea());
	
	//negative approach - create rock tiles first, then make sure all accessible tiles have no rock
	auto rockArea = zone.area().getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
	
	map.getEditManager()->getTerrainSelection().setSelection(rockArea.getTilesVector());
	map.getEditManager()->drawTerrain(rockTerrain, &generator.rand);
	
	//now make sure all accessible tiles have no additional rock on them
	map.getEditManager()->getTerrainSelection().setSelection(accessibleArea.getTilesVector());
	map.getEditManager()->drawTerrain(zone.getTerrainType(), &generator.rand);
	
	//finally mark rock tiles as occupied, spawn no obstacles there
	rockArea = zone.area().getSubarea([this](const int3 & t)
	{
		return !map.map().getTile(t).terType.isPassable();
	});
	zone.areaUsed().unite(rockArea);
	if(auto * m = zone.getModificator<RiverPlacer>())
		m->riverProhibit().unite(rockArea);
}

void RockPlacer::init()
{
	postfunction(zone.getModificator<RoadPlacer>());
	dependency(zone.getModificator<TreasurePlacer>());
}
