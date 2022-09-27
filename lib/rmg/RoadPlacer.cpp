/*
 * RoadPlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RoadPlacer.h"
#include "ObjectManager.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "RmgPath.h"

VCMI_LIB_NAMESPACE_BEGIN

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

const rmg::Area & RoadPlacer::getRoads() const
{
	return roads;
}

bool RoadPlacer::createRoad(const int3 & dst)
{
	auto searchArea = zone.areaPossible() + areaRoads + zone.freePaths() - isolated + roads;
	
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

void RoadPlacer::drawRoads(bool secondary)
{
	zone.areaPossible().subtract(roads);
	zone.freePaths().unite(roads);
	map.getEditManager()->getTerrainSelection().setSelection(roads.getTilesVector());
	std::string roadCode = (secondary ? generator.getConfig().secondaryRoadType : generator.getConfig().defaultRoadType);
	TRoadId roadType = VLC->terrainTypeHandler->getRoadByCode(roadCode)->id;
	map.getEditManager()->drawRoad(roadType, &generator.rand);
}

void RoadPlacer::addRoadNode(const int3& node)
{
	roadNodes.insert(node);
}

void RoadPlacer::connectRoads()
{
	bool noRoadNodes = false;
	//Assumes objects are already placed
	if (roadNodes.size() < 2)
	{
		//If there are no nodes, draw roads to mines
		noRoadNodes = true;
		if (auto* m = zone.getModificator<ObjectManager>())
		{
			for (auto object : m->getMines())
			{
				addRoadNode(object->visitablePos());
			}
		}
	}

	if(roadNodes.size() < 2)
		return;
	
	//take any tile from road nodes as destination zone for all other road nodes
	if(roads.empty())
		roads.add(*roadNodes.begin());
	
	for(auto & node : roadNodes)
	{
		createRoad(node);
	}
	
	//Draw dirt roads if there are only mines
	drawRoads(noRoadNodes);
}

char RoadPlacer::dump(const int3 & t)
{
	if(roadNodes.count(t))
		return '@';
	if(roads.contains(t))
		return '+';
	if(isolated.contains(t))
		return 'i';
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
