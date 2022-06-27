/*
 * RoadPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "RoadPlacer.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "RmgPath.h"

RoadPlacer::RoadPlacer(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void RoadPlacer::process()
{
	connectRoads();
}

bool RoadPlacer::createRoad(const rmg::Area & dst)
{
	rmg::Path path(zone.freePaths() + zone.areaPossible());
	path.connect(roads);
	auto res = path.search(dst, true);
	if(res.getPathArea().empty())
	{
		auto res = path.search(dst, false);
		if(res.getPathArea().empty())
		{
			logGlobal->warn("Failed to create road");
			return false;
		}
	}
	roads.unite(res.getPathArea());
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
	map.getEditManager()->drawRoad(ROAD_NAMES[1], &generator.rand);
}

void RoadPlacer::addRoadNode(const int3& node)
{
	roadNodes.add(node);
}

void RoadPlacer::connectRoads()
{
	logGlobal->debug("Started building roads");
	if(roadNodes.empty())
		return;
	
	roads.add(*roadNodes.getTiles().begin());
	
	auto areas = connectedAreas(roadNodes);
	for(auto & area : areas)
	{
		createRoad(area);
	}
	
	drawRoads();
	
	logGlobal->debug("Finished building roads");
}
