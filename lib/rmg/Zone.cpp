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

VCMI_LIB_NAMESPACE_BEGIN

std::function<bool(const int3 &)> AREA_NO_FILTER = [](const int3 & t)
{
	return true;
};

Zone::Zone(RmgMap & map, CMapGenerator & generator, CRandomGenerator & r)
	: finished(false)
	, townType(ETownType::NEUTRAL)
	, terrainType(ETerrainId::GRASS)
	, map(map)
	, generator(generator)
{
	rand.setSeed(r.nextInt());
}

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

const rmg::Area & Zone::getArea() const
{
	return dArea;
}

rmg::Area & Zone::area()
{
	return dArea;
}

rmg::Area & Zone::areaPossible()
{
	//FIXME: make const, only modify via mutex-protected interface
	return dAreaPossible;
}

rmg::Area & Zone::areaUsed()
{
	return dAreaUsed;
}

void Zone::clearTiles()
{
	//Lock lock(mx);
	dArea.clear();
	dAreaPossible.clear();
	dAreaFree.clear();
}

void Zone::initFreeTiles()
{
	rmg::Tileset possibleTiles;
	//Lock lock(mx);
	vstd::copy_if(dArea.getTiles(), vstd::set_inserter(possibleTiles), [this](const int3 &tile) -> bool
	{
		return map.isPossible(tile);
	});
	dAreaPossible.assign(possibleTiles);
	
	if(dAreaFree.empty())
	{
		dAreaPossible.erase(pos);
		dAreaFree.add(pos); //zone must have at least one free tile where other paths go - for instance in the center
	}
}

rmg::Area & Zone::freePaths()
{
	return dAreaFree;
}

FactionID Zone::getTownType() const
{
	return FactionID(townType);
}

void Zone::setTownType(si32 town)
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
	dAreaPossible.subtract(path.getPathArea());
	dAreaFree.unite(path.getPathArea());
	for(const auto & t : path.getPathArea().getTilesVector())
		map.setOccupied(t, ETileType::FREE);
}

void Zone::fractalize()
{
	rmg::Area clearedTiles(dAreaFree);
	rmg::Area possibleTiles(dAreaPossible);
	rmg::Area tilesToIgnore; //will be erased in this iteration

	//Squared
	float minDistance = 10 * 10;
	float spanFactor = (pos.z ? 0.25 : 0.5f); //Narrower passages in the Underground

	int treasureValue = 0;
	int treasureDensity = 0;
	for (auto t : treasureInfo)
	{
		treasureValue += ((t.min + t.max) / 2) * t.density / 1000.f; //Thousands
		treasureDensity += t.density;
	}

	if (treasureValue > 200)
	{
		//Less obstacles - max span is 1 (no obstacles)
		spanFactor = 1.0f - ((std::max(0, (1000 - treasureValue)) / (1000.f - 200)) * (1 - spanFactor));
	}
	else if (treasureValue < 100)
	{
		//Dense obstacles
		spanFactor *= (treasureValue / 100.f);
		vstd::amax(spanFactor, 0.2f);
	}
	if (treasureDensity <= 10)
	{
		vstd::amin(spanFactor, 0.25f); //Add extra obstacles to fill up space
	}
	float blockDistance = minDistance * spanFactor; //More obstacles in the Underground
	
	if(type != ETemplateZoneType::JUNCTION)
	{
		//junction is not fractalized, has only one straight path
		//everything else remains blocked
		while(!possibleTiles.empty())
		{
			//link tiles in random order
			std::vector<int3> tilesToMakePath = possibleTiles.getTilesVector();
			RandomGeneratorUtil::randomShuffle(tilesToMakePath, getRand());
			
			int3 nodeFound(-1, -1, -1);

			for(const auto & tileToMakePath : tilesToMakePath)
			{
				//find closest free tile
				int3 closestTile = clearedTiles.nearest(tileToMakePath);
				if(closestTile.dist2dSQ(tileToMakePath) <= minDistance)
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
			if(!nodeFound.valid()) //nothing else can be done (?)
				break;
			tilesToIgnore.clear();
		}
	}
	Lock lock(areaMutex);
	//cut straight paths towards the center. A* is too slow for that.
	auto areas = connectedAreas(clearedTiles, false);
	for(auto & area : areas)
	{
		if(dAreaFree.overlap(area))
			continue; //already found
			
		auto availableArea = dAreaPossible + dAreaFree;
		rmg::Path path(availableArea);
		path.connect(dAreaFree);
		auto res = path.search(area, false);
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
	logGlobal->info("Zone %d modificators initialized", getId());
}

CRandomGenerator& Zone::getRand()
{
	return rand;
}

VCMI_LIB_NAMESPACE_END
