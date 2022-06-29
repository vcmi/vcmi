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

void RoadPlacer::process()
{
	connectRoads();
}

bool RoadPlacer::createRoad(const int3 & dst)
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
	
	map.getEditManager()->getTerrainSelection().setSelection(tiles);
	map.getEditManager()->drawRoad(generator.getConfig().defaultRoadType, &generator.rand);
}

void RoadPlacer::addRoadNode(const int3& node)
{
	roadNodes.insert(node);
}

void RoadPlacer::connectRoads()
{
	if(roadNodes.empty())
		return;
	
	//take any tile from road nodes as destination zone for all other road nodes
	if(roads.empty())
		roads.add(*roadNodes.begin());
	
	for(auto & node : roadNodes)
	{
		createRoad(node);
	}
	
	drawRoads();
}

char RoadPlacer::dump(const int3 & t)
{
	if(roadNodes.count(t))
		return '@';
	if(roads.contains(t))
		return '+';
	return Modificator::dump(t);
}
