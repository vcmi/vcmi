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

float RoadPlacer::isolatedMovePenalty(const int3 & src, const int3 & dst)
{
	if(std::abs((src - dst).y) == 1)
	{
		//Do not allow connections straight up through object not visitable from top
		if(areaIsolated().contains(dst) || areaIsolated().contains(src))
			return 1e12f;
	}
	else if(areaIsolated().contains(dst))
	{
		//Simply do not route road behind objects that are not visitable from top, such as Monoliths
		return 1e6f;
	}
	return 0.f;
}

bool RoadPlacer::createRoad(const int3 & destination, const rmg::DistanceMap & borderDistance)
{
	auto searchArea = zone.areaPossible() + zone.freePaths() + areaRoads + roads;

	rmg::Area border(zone.area()->getBorder());

	rmg::Path path(searchArea);
	path.connect(roads);

	auto simpleRoutig = [this, &border](const int3& src, const int3& dst)
	{
		if(const float penalty = isolatedMovePenalty(src, dst); penalty > 0.f)
			return penalty;

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

	auto res = rmg::Path::invalid();
	if(const auto it = nodeCurves.find(destination); it != nodeCurves.end())
	{
		constexpr int JOIN_DISTANCE = 8; // tiles from an existing road over which curve deviation fades in
		rmg::DistanceMap distFromRoads;
		rmg::Area grown = roads;
		for(const auto & tile : roads.getTilesVector())
			distFromRoads[tile] = 0;
		for(int r = 1; r <= JOIN_DISTANCE; ++r)
		{
			rmg::Area ring(grown.getBorderOutside());
			grown.unite(ring);
			for(const auto & tile : ring.getTilesVector())
				distFromRoads[tile] = r;
		}

		const auto & curve = it->second;
		auto curveRoutig = [this, &curve, &borderDistance, &distFromRoads](const int3 & src, const int3 & dst) -> float
		{
			if(const float penalty = isolatedMovePenalty(src, dst); penalty > 0.f)
				return penalty;

			float w = 1.f;
			if(const auto rit = distFromRoads.find(dst); rit != distFromRoads.end())
			{
				const float r = static_cast<float>(rit->second);
				w = (r / JOIN_DISTANCE) * (r / JOIN_DISTANCE);
			}

			const float distToCurve = curve.distanceSq(dst);
			float ret = 1.f + w * distToCurve * distToCurve + curve.borderPenalty(borderDistance, dst);
			if(visitableTiles.contains(src) || visitableTiles.contains(dst))
				ret *= VISITABLE_PENALTY;
			return ret;
		};
		res = path.search(destination, true, curveRoutig);
	}

	if(!res.valid())
		res = path.search(destination, true, simpleRoutig);

	if(!res.valid())
	{
		res = createRoadDesperate(path, destination);
		if (!res.valid())
		{
			logRmg->warn("Failed to create road to node %s", destination.toString());
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
		if(const float penalty = isolatedMovePenalty(src, dst); penalty > 0.f)
			return penalty;

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

void RoadPlacer::setNodeCurve(const int3 & node, const rmg::BezierCurve & curve)
{
	RecursiveLock lock(externalAccessMutex);
	int3 key = node;
	if(!roadNodes.contains(key))
	{
		// Guarded objects pass the guard tile as src; the road node is the object, one step away.
		bool found = false;
		for(const auto & dir : int3::getDirs())
		{
			const int3 candidate = node + dir;
			if(roadNodes.contains(candidate))
			{
				key = candidate;
				found = true;
				break;
			}
		}
		if(!found)
			return;
	}
	nodeCurves[key] = curve;
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
	
	// Seed at the zone position (main town visitable tile, or zone centre).
	RecursiveLock lock(externalAccessMutex);
	if(roads.empty())
		roads.add(zone.getPos());

	logRmg->debug("Zone %d: %d of %d road nodes have a curve", zone.getId(), static_cast<int>(nodeCurves.size()), static_cast<int>(roadNodes.size()));

	std::map<int, rmg::Tileset> reverse;
	const auto borderDistance = zone.area()->computeDistanceMap(reverse);

	for(const auto & node : roadNodes)
	{
		try
		{
			createRoad(node, borderDistance);
		}
		catch (const rmgException& e)
		{
			logRmg->warn("Handled exception while drawing road to node %s: %s", node.toString(), e.what());
		}
		catch (const std::exception & e)
		{
			logRmg->error("Unhandled exception while drawing road to node %s: %s", node.toString(), e.what());
			throw;
		}
	}

	roads = pruneRoadDeadEnds(roads, roadNodes);
	
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

std::string RoadPlacer::dumpLegend() const
{
	return "@ road node\n"
		"+ road\n"
		"i isolated\n"
		+ Modificator::dumpLegend();
}
