//
//  RoadPlacer.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "RoadPlacer.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "CRmgPath.h"

RoadPlacer::RoadPlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void RoadPlacer::process()
{
	connectRoads();
}

bool RoadPlacer::createRoad(const Rmg::Area & dst)
{
	Rmg::Path path(zone.freePaths() + zone.areaPossible());
	path.connect(roads);
	path.search(dst, true);
	if(path.getPathArea().empty())
	{
		path.search(dst, false);
		if(path.getPathArea().empty())
		{
			logGlobal->warn("Failed to create road");
			roads.unite(dst);
			return false;
		}
	}
	roads.unite(dst);
	roads.unite(path.getPathArea());
	return true;
	
}

void RoadPlacer::drawRoads()
{
	std::vector<int3> tiles;
	for (auto tile : roads.getTiles())
	{
		if(map.isOnMap(tile))
			tiles.push_back (tile);
	}
	for (auto tile : roadNodes.getTiles())
	{
		if (map.getZoneID(tile) == zone.getId()) //mark roads for our nodes, but not for zone guards in other zones
			tiles.push_back(tile);
	}
	
	map.getEditManager()->getTerrainSelection().setSelection(tiles);
	//map.getEditManager()->drawRoad(gen.getConfig().defaultRoadType, &generator);
	map.getEditManager()->drawRoad(ROAD_NAMES[1], &generator);
}

void RoadPlacer::addRoadNode(const int3& node)
{
	roadNodes.add(node);
}

void RoadPlacer::connectRoads()
{
	logGlobal->debug("Started building roads");
	
	auto areas = connectedAreas(roadNodes);
	for(auto & area : areas)
	{
		createRoad(area);
	}
	
	drawRoads();
	
	logGlobal->debug("Finished building roads");
}
