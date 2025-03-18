/*
 * Zone.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Zone.h"
#include "RmgMap.h"
#include "Functions.h"
#include "TileInfo.h"
#include "CMapGenerator.h"
#include "RmgPath.h"
#include "modificators/ObjectManager.h"

#include "../CRandomGenerator.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

const std::function<bool(const int3 &)> AREA_NO_FILTER = [](const int3 & t)
{
	return true;
};

Zone::Zone(RmgMap & map, CMapGenerator & generator, vstd::RNG & r)
	: finished(false)
	, townType(ETownType::NEUTRAL)
	, terrainType(ETerrainId::GRASS)
	, map(map)
	, rand(std::make_unique<CRandomGenerator>(r.nextInt()))
	, generator(generator)
{
}

Zone::~Zone() = default;

bool Zone::isUnderground() const
{
	return getPos().z;
}

void Zone::setOptions(const ZoneOptions& options)
{
	ZoneOptions::operator=(options);
}

float3 Zone::getCenter() const
{
	return center;
}

void Zone::setCenter(const float3 &f)
{
	//limit boundaries to (0,1) square
	
	//alternate solution - wrap zone around unitary square. If it doesn't fit on one side, will come out on the opposite side
	center = f;
	
	center.x = static_cast<float>(std::fmod(center.x, 1));
	center.y = static_cast<float>(std::fmod(center.y, 1));
	
	if(center.x < 0) //fmod seems to work only for positive numbers? we want to stay positive
		center.x = 1 - std::abs(center.x);
	if(center.y < 0)
		center.y = 1 - std::abs(center.y);
}

int3 Zone::getPos() const
{
	return pos;
}

void Zone::setPos(const int3 &Pos)
{
	pos = Pos;
}

ThreadSafeProxy<rmg::Area> Zone::area()
{
	return ThreadSafeProxy<rmg::Area>(dArea, areaMutex);
}

ThreadSafeProxy<const rmg::Area> Zone::area() const
{
	return ThreadSafeProxy<const rmg::Area>(dArea, areaMutex);
}

ThreadSafeProxy<rmg::Area> Zone::areaPossible()
{
	return ThreadSafeProxy<rmg::Area>(dAreaPossible, areaMutex);
}

ThreadSafeProxy<const rmg::Area> Zone::areaPossible() const
{
	return ThreadSafeProxy<const rmg::Area>(dAreaPossible, areaMutex);
}

ThreadSafeProxy<rmg::Area> Zone::freePaths()
{
	return ThreadSafeProxy<rmg::Area>(dAreaFree, areaMutex);
}

ThreadSafeProxy<const rmg::Area> Zone::freePaths() const
{
	return ThreadSafeProxy<const rmg::Area>(dAreaFree, areaMutex);
}

ThreadSafeProxy<rmg::Area> Zone::areaUsed()
{
	return ThreadSafeProxy<rmg::Area>(dAreaUsed, areaMutex);
}

ThreadSafeProxy<const rmg::Area> Zone::areaUsed() const
{
	return ThreadSafeProxy<const rmg::Area>(dAreaUsed, areaMutex);
}

rmg::Area Zone::areaForRoads() const
{
	return areaPossible() + freePaths();
}

void Zone::clearTiles()
{
	Lock lock(areaMutex);
	dArea.clear();
	dAreaPossible.clear();
	dAreaFree.clear();
}

void Zone::initFreeTiles()
{
	rmg::Tileset possibleTiles;
	vstd::copy_if(dArea.getTiles(), vstd::set_inserter(possibleTiles), [this](const int3 &tile) -> bool
	{
		return map.isPossible(tile);
	});
	dAreaPossible.assign(possibleTiles);
	
	if(dAreaFree.empty() && getType() != ETemplateZoneType::SEALED)
	{
		// Fixme: This might fail fot water zone, which doesn't need to have a tile in its center of the mass
		dAreaPossible.erase(pos);
		dAreaFree.add(pos); //zone must have at least one free tile where other paths go - for instance in the center
	}
}

FactionID Zone::getTownType() const
{
	return townType;
}

void Zone::setTownType(FactionID town)
{
	townType = town;
}

TerrainId Zone::getTerrainType() const
{
	return terrainType;
}

void Zone::setTerrainType(TerrainId terrain)
{
	terrainType = terrain;
}

void Zone::moveToCenterOfMass()
{
	auto newPos = area()->getCenterOfMass();
	setPos(newPos);
	setCenter(float3(float(newPos.x) / map.getMapGenOptions().getWidth(), 
		float(newPos.y) / map.getMapGenOptions().getHeight(), 
		newPos.z));
}

rmg::Path Zone::searchPath(const rmg::Area & src, bool onlyStraight, const std::function<bool(const int3 &)> & areafilter) const
///connect current tile to any other free tile within zone
{
	auto movementCost = [this](const int3 & s, const int3 & d)
	{
		if(map.isFree(d))
			return 1;
		else if (map.isPossible(d))
			return 2;
		return 3;
	};

	auto area = (dAreaPossible + dAreaFree).getSubarea(areafilter);
	rmg::Path freePath(area);
	rmg::Path resultPath(area);
	freePath.connect(dAreaFree);

	//connect to all pieces
	auto goals = connectedAreas(src, onlyStraight);
	for(auto & goal : goals)
	{
		auto path = freePath.search(goal, onlyStraight, movementCost);
		if(path.getPathArea().empty())
			return rmg::Path::invalid();

		freePath.connect(path.getPathArea());
		resultPath.connect(path.getPathArea());
	}

	return resultPath;
}

rmg::Path Zone::searchPath(const rmg::Area & src, bool onlyStraight, const rmg::Area & searchArea) const
///connect current tile to any other free tile within searchArea
{
	auto movementCost = [this](const int3 & s, const int3 & d)
	{
		if(map.isFree(d))
			return 1;
		else if (map.isPossible(d))
			return 2;
		return 3;
	};

	rmg::Path freePath(searchArea);
	rmg::Path resultPath(searchArea);
	freePath.connect(dAreaFree);

	//connect to all pieces
	auto goals = connectedAreas(src, onlyStraight);
	for(auto & goal : goals)
	{
		auto path = freePath.search(goal, onlyStraight, movementCost);
		if(path.getPathArea().empty())
			return rmg::Path::invalid();

		freePath.connect(path.getPathArea());
		resultPath.connect(path.getPathArea());
	}

	return resultPath;
}


rmg::Path Zone::searchPath(const int3 & src, bool onlyStraight, const std::function<bool(const int3 &)> & areafilter) const
///connect current tile to any other free tile within zone
{
	return searchPath(rmg::Area({ src }), onlyStraight, areafilter);
}

TModificators Zone::getModificators()
{
	return modificators;
}

void Zone::connectPath(const rmg::Path & path)
///connect current tile to any other free tile within zone
{
	areaPossible()->subtract(path.getPathArea());
	freePaths()->unite(path.getPathArea());
	for(const auto & t : path.getPathArea().getTilesVector())
		map.setOccupied(t, ETileType::FREE);
}

void Zone::fractalize()
{
	//Squared
	float minDistance = 9 * 9;
	float freeDistance = pos.z ? (10 * 10) : (9 * 9);
	float spanFactor = (pos.z ? 0.3f : 0.45f); //Narrower passages in the Underground
	float marginFactor = 1.0f;

	int treasureValue = 0;
	int treasureDensity = 0;
	for (const auto & t : treasureInfo)
	{
		treasureValue += ((t.min + t.max) / 2) * t.density / 1000.f; //Thousands
		treasureDensity += t.density;
	}

	if (getType() == ETemplateZoneType::WATER)
	{
		// Set very little obstacles on water
		spanFactor = 0.2;
	}
	else //Scale with treasure density
	{
		if (treasureValue > 250)
		{
			// A quarter at max density - means more free space
			marginFactor = (0.6f + ((std::max(0, (600 - treasureValue))) / (600.f - 250)) * 0.4f);

			// Low value - dense obstacles
			spanFactor *= (0.6f + ((std::max(0, (600 - treasureValue))) / (600.f - 250)) * 0.4f);
		}
		else if (treasureValue < 100)
		{
			//Dense obstacles
			spanFactor *= (0.5 + 0.5 * (treasureValue / 100.f));
			vstd::amax(spanFactor, 0.15f);
		}
		if (treasureDensity <= 10)
		{
			vstd::amin(spanFactor, 0.1f + 0.01f * treasureDensity); //Add extra obstacles to fill up space
	}
	}

	float blockDistance = minDistance * spanFactor; //More obstacles in the Underground
	freeDistance = freeDistance * marginFactor;
	vstd::amax(freeDistance, 4 * 4);
	logGlobal->trace("Zone %d: treasureValue %d blockDistance: %2.f, freeDistance: %2.f", getId(), treasureValue, blockDistance, freeDistance);

	Lock lock(areaMutex);

	rmg::Area clearedTiles(dAreaFree);
	rmg::Area possibleTiles(dAreaPossible);
	rmg::Area tilesToIgnore; //will be erased in this iteration
	
	if(type != ETemplateZoneType::JUNCTION)
	{
		//junction is not fractalized, has only one straight path
		//everything else remains blocked
		while(!possibleTiles.empty())
		{
			//link tiles in random order
			std::vector<int3> tilesToMakePath = possibleTiles.getTilesVector();

			// Do not fractalize tiles near the edge of the map to avoid paths adjacent to map edge
			const auto h = map.height();
			const auto w = map.width();
			const size_t MARGIN = 3;
			vstd::erase_if(tilesToMakePath, [&, h, w](const int3 & tile)
			{
				return tile.x < MARGIN || tile.x > (w - MARGIN) ||
					tile.y < MARGIN || tile.y > (h - MARGIN);
			});
			RandomGeneratorUtil::randomShuffle(tilesToMakePath, getRand());
			
			int3 nodeFound(-1, -1, -1);

			for(const auto & tileToMakePath : tilesToMakePath)
			{
				//find closest free tile
				int3 closestTile = clearedTiles.nearest(tileToMakePath);
				if(closestTile.dist2dSQ(tileToMakePath) <= freeDistance)
					tilesToIgnore.add(tileToMakePath);
				else
				{
					//if tiles are not close enough, make path to it
					nodeFound = tileToMakePath;
					clearedTiles.add(nodeFound); //from now on nearby tiles will be considered handled
					break; //next iteration - use already cleared tiles
				}
			}
			
			possibleTiles.subtract(tilesToIgnore);
			if(!nodeFound.isValid()) //nothing else can be done (?)
				break;
			tilesToIgnore.clear();
		}
	}
	else if (type == ETemplateZoneType::SEALED)
	{
		//Completely block all the tiles in the zone
		auto tiles = areaPossible()->getTiles();
		for(const auto & t : tiles)
			map.setOccupied(t, ETileType::BLOCKED);
		possibleTiles.clear();
		dAreaFree.clear();
		return;
	}
	else
	{
		// Handle special case - place Monoliths at the edge of a zone
		auto objectManager = getModificator<ObjectManager>();
		if (objectManager)
		{
			objectManager->createMonoliths();
		}
	}

	//Connect with free areas
	auto areas = connectedAreas(clearedTiles, true);
	for(auto & area : areas)
	{
		if(dAreaFree.overlap(area))
			continue; //already found
			
		auto availableArea = dAreaPossible + dAreaFree;
		rmg::Path path(availableArea);
		path.connect(dAreaFree);
		auto res = path.search(area, true);
		if(res.getPathArea().empty())
		{
			dAreaPossible.subtract(area);
			dAreaFree.subtract(area);
			for(const auto & t : area.getTiles())
				map.setOccupied(t, ETileType::BLOCKED);
		}
		else
		{
			dAreaPossible.subtract(res.getPathArea());
			dAreaFree.unite(res.getPathArea());
			for(const auto & t : res.getPathArea().getTiles())
				map.setOccupied(t, ETileType::FREE);
		}
	}
	
	//now block most distant tiles away from passages
	auto areaToBlock = dArea.getSubarea([this, blockDistance](const int3 & t)
	{
		auto distance = static_cast<float>(dAreaFree.distanceSqr(t));
		return distance > blockDistance;
	});
	dAreaPossible.subtract(areaToBlock);
	dAreaFree.subtract(areaToBlock);

	lock.unlock();
	for(const auto & t : areaToBlock.getTiles())
		map.setOccupied(t, ETileType::BLOCKED);
}

void Zone::initModificators()
{
	for(auto & modificator : modificators)
	{
		modificator->init();
	}
}

vstd::RNG& Zone::getRand()
{
	return *rand;
}

VCMI_LIB_NAMESPACE_END
