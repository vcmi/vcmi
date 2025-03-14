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
#include "../MapProxy.h"
#include "../../mapping/CMapEditManager.h"
#include "../../mapObjects/CGObjectInstance.h"
#include "../../modding/IdentifierStorage.h"
#include "../../modding/ModScope.h"
#include "../../TerrainHandler.h"
#include "../../GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

class TerrainType;

void RoadPlacer::process()
{
	if(generator.getConfig().defaultRoadType.empty() && generator.getConfig().secondaryRoadType.empty())
		return; //do not generate roads at all
	
	connectRoads();
}

void RoadPlacer::postProcess()
{
	//Draw dirt roads if there are only mines
	drawRoads(noRoadNodes);
}

void RoadPlacer::init()
{
}

rmg::Area & RoadPlacer::areaForRoads()
{
	return areaRoads;
}

rmg::Area & RoadPlacer::areaIsolated()
{
	return isolated;
}

rmg::Area & RoadPlacer::areaVisitable()
{
	return visitableTiles;
}

const rmg::Area & RoadPlacer::getRoads() const
{
	return roads;
}

bool RoadPlacer::createRoad(const int3 & destination)
{
	auto searchArea = zone.areaPossible() + zone.freePaths() + areaRoads + roads;

	rmg::Area border(zone.area()->getBorder());

	rmg::Path path(searchArea);
	path.connect(roads);

	auto simpleRoutig = [this, &border](const int3& src, const int3& dst)
	{
		if(std::abs((src - dst).y) == 1)
		{
			//Do not allow connections straight up through object not visitable from top
			if(areaIsolated().contains(dst) || areaIsolated().contains(src))
			{
				return 1e12f;
			}
		}
		else
		{
			if(areaIsolated().contains(dst))
			{
				//Simply do not route road behind objects that are not visitable from top, such as Monoliths
				return 1e6f;
			}
		}

		float ret = rmg::Path::nonEuclideanCostFunction(src, dst, border.getCenterOfMass());

		if (visitableTiles.contains(src) || visitableTiles.contains(dst))
		{
			ret *= VISITABLE_PENALTY;
		}
		float dist = border.distanceSqr(dst);
		if(dist > 1.0f)
		{
			ret /= dist;
		}
		return ret;
	};
	
	auto res = path.search(destination, true, simpleRoutig);
	if(!res.valid())
	{
		res = createRoadDesperate(path, destination);
		if (!res.valid())
		{
			logGlobal->warn("Failed to create road to node %s", destination.toString());
			return false;
		}
	}
	roads.unite(res.getPathArea());
	return true;
	
}

rmg::Path RoadPlacer::createRoadDesperate(rmg::Path & path, const int3 & destination)
{
	auto desperateRoutig = [this](const int3& src, const int3& dst) -> float
	{
		//Do not allow connections straight up through object not visitable from top
		if(std::abs((src - dst).y) == 1)
		{
			if(areaIsolated().contains(dst) || areaIsolated().contains(src))
			{
				return 1e12;
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
		auto ret =  weight * weight; // Still prefer straight paths

		if (visitableTiles.contains(src) || visitableTiles.contains(dst))
		{
			ret *= VISITABLE_PENALTY;
		}
		return ret;
	};
	return path.search(destination, false, desperateRoutig);
}

void RoadPlacer::drawRoads(bool secondary)
{	
	//Do not draw roads on underground rock or water
	roads.erase_if([this](const int3& pos) -> bool
	{
		const auto* terrain = map.getTile(pos).getTerrain();
		return !terrain->isPassable() || !terrain->isLand();
	});

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
	RoadId roadType(*LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "road", roadName));

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
			throw;
		}
	}
	
	if (!zone.isUnderground())
	{
		// Otherwise roads will be drawn only after rock is placed
		postProcess();
	}
}

char RoadPlacer::dump(const int3 & t)
{
	if(vstd::contains(roadNodes, t))
		return '@';
	if(roads.contains(t))
		return '+';
	if(isolated.contains(t))
		return 'i';
	return Modificator::dump(t);
}

VCMI_LIB_NAMESPACE_END
