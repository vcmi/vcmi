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
#include "ObstaclePlacer.h"
#include "RockFiller.h"
#include "../Functions.h"
#include "../CMapGenerator.h"
#include "../threadpool/MapProxy.h"
#include "../../CModHandler.h"
#include "../../mapping/CMapEditManager.h"
#include "../../TerrainHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class TerrainType;

void RoadPlacer::process()
{
	if(generator.getConfig().defaultRoadType.empty() && generator.getConfig().secondaryRoadType.empty())
		return; //do not generate roads at all
	
	connectRoads();
}

void RoadPlacer::init()
{
	if(zone.isUnderground())
	{
		DEPENDENCY_ALL(RockFiller);
	}
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
	auto searchArea = zone.areaPossible() + zone.freePaths() + areaRoads + roads;

	rmg::Path path(searchArea);
	path.connect(roads);

	auto simpleRoutig = [this](const int3& src, const int3& dst)
	{
		if(areaIsolated().contains(dst))
		{
			return 1000.0f; //Do not route road behind objects that are not visitable from top
		}
		else
		{
			return 1.0f;
		}
	};
	
	auto res = path.search(dst, true, simpleRoutig);
	if(!res.valid())
	{
		auto desperateRoutig = [this](const int3& src, const int3& dst) -> float
		{
			//Do not allow connections straight up through object not visitable from top
			if(std::abs((src - dst).y) == 1)
			{
				if(areaIsolated().contains(dst) || areaIsolated().contains(src))
				{
					return 1e30;
				}
			}
			else
			{
				if(areaIsolated().contains(dst))
				{
					return 1e6;
				}
			}

			float weight = dst.dist2dSQ(src);
			return weight * weight;
		};
		res = path.search(dst, false, desperateRoutig);

		if(!res.valid())
		{
			logGlobal->warn("Failed to create road to node %s", dst.toString());
			return false;
		}
	}
	roads.unite(res.getPathArea());
	return true;
	
}

void RoadPlacer::drawRoads(bool secondary)
{	
	{
		//Clean space under roads even if they won't be eventually generated
		Zone::Lock lock(zone.areaMutex);

		//Do not draw roads on underground rock or water
		roads.erase_if([this](const int3& pos) -> bool
		{
			const auto* terrain = map.getTile(pos).terType;;
			return !terrain->isPassable() || !terrain->isLand();
		});

		zone.areaPossible().subtract(roads);
		zone.freePaths().unite(roads);
	}

	if(!generator.getMapGenOptions().isRoadEnabled())
	{
		return;
	}

	if((secondary && generator.getConfig().secondaryRoadType.empty())
		|| (!secondary && generator.getConfig().defaultRoadType.empty()))
		return;

	//TODO: Allow custom road type for object
	//TODO: Remove these default types

	auto tiles = roads.getTilesVector();

	std::string roadName = (secondary ? generator.getConfig().secondaryRoadType : generator.getConfig().defaultRoadType);
	RoadId roadType(*VLC->modh->identifiers.getIdentifier(CModHandler::scopeGame(), "road", roadName));

	//If our road type is not enabled, choose highest below it
	for (int8_t bestRoad = roadType.getNum(); bestRoad > RoadId(Road::NO_ROAD).getNum(); bestRoad--)
	{
		if(generator.getMapGenOptions().isRoadEnabled(RoadId(bestRoad)))
		{
			mapProxy->drawRoads(zone.getRand(), tiles, RoadId(bestRoad));
			return;
		}
	}
}

void RoadPlacer::addRoadNode(const int3& node)
{
	RecursiveLock lock(externalAccessMutex);
	roadNodes.insert(node);
}

void RoadPlacer::connectRoads()
{
	bool noRoadNodes = false;
	//Assumes objects are already placed
	if(roadNodes.size() < 2)
	{
		//If there are no nodes, draw roads to mines
		noRoadNodes = true;
		if(auto* m = zone.getModificator<ObjectManager>())
		{
			for(auto * object : m->getMines())
			{
				addRoadNode(object->visitablePos());
			}
		}
	}

	if(roadNodes.size() < 2)
		return;
	
	//take any tile from road nodes as destination zone for all other road nodes
	RecursiveLock lock(externalAccessMutex);
	if(roads.empty())
		roads.add(*roadNodes.begin());

	for(const auto & node : roadNodes)
	{
		try
		{
			createRoad(node);
		}
		catch (const rmgException& e)
		{
			logGlobal->warn("Handled exception while drawing road to node %s: %s", node.toString(), e.what());
		}
		catch (const std::exception & e)
		{
			logGlobal->error("Unhandled exception while drawing road to node %s: %s", node.toString(), e.what());
			throw e;
		}
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
