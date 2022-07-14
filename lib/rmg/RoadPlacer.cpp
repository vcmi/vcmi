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

rmg::Area & RoadPlacer::areaForRoads()
{
	return areaRoads;
}

rmg::Area & RoadPlacer::areaIsolated()
{
	return isolated;
}

bool RoadPlacer::createRoad(const int3 & dst)
{
	auto searchArea = zone.areaPossible() - isolated + zone.freePaths() + areaRoads;
	
	rmg::Path path(searchArea);
	path.connect(roads);
	
	auto res = path.search(dst, true);
	if(!res.valid())
	{
		res = path.search(dst, false, [](const int3 & src, const int3 & dst)
		{
			float weight = dst.dist2dSQ(src);
			return weight * weight;
		});
		if(!res.valid())
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
	zone.freePaths().unite(roads);
	map.getEditManager()->getTerrainSelection().setSelection(roads.getTilesVector());
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
