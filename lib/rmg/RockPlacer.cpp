//
//  RockPlacer.cpp
//  vcmi
//
//  Created by nordsoft on 06.07.2022.
//

#include "RockPlacer.h"
#include "TreasurePlacer.h"
#include "RoadPlacer.h"
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
	zone.areaUsed().unite(rockArea.getSubarea([this](const int3 & t)
	{
		return !map.map().getTile(t).terType.isPassable();
	}));
}

void RockPlacer::init()
{
	postfunction(zone.getModificator<RoadPlacer>());
	dependency(zone.getModificator<TreasurePlacer>());
}
