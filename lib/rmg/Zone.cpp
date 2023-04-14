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
#include "../mapping/CMap.h"
#include "../CStopWatch.h"
#include "CMapGenerator.h"
#include "RmgPath.h"

VCMI_LIB_NAMESPACE_BEGIN

std::function<bool(const int3 &)> AREA_NO_FILTER = [](const int3 & t)
{
	return true;
};

Zone::Zone(RmgMap & map, CMapGenerator & generator)
	: townType(ETownType::NEUTRAL)
	, terrainType(ETerrainId::GRASS)
	, map(map)
	, generator(generator)
{
	
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
	return dAreaPossible;
}

rmg::Area & Zone::areaUsed()
{
	return dAreaUsed;
}

void Zone::clearTiles()
{
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

si32 Zone::getTownType() const
{
	return townType;
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
	return searchPath(rmg::Area({src}), onlyStraight, areafilter);
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

	const float minDistance = 10 * 10; //squared
	
	if(type != ETemplateZoneType::JUNCTION)
	{
		//junction is not fractalized, has only one straight path
		//everything else remains blocked
		while(!possibleTiles.empty())
		{
			//link tiles in random order
			std::vector<int3> tilesToMakePath = possibleTiles.getTilesVector();
			RandomGeneratorUtil::randomShuffle(tilesToMakePath, generator.rand);
			
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
	float blockDistance = minDistance * 0.25f;
	auto areaToBlock = dArea.getSubarea([this, blockDistance](const int3 & t)
	{
		auto distance = static_cast<float>(dAreaFree.distanceSqr(t));
		return distance > blockDistance;
	});
	dAreaPossible.subtract(areaToBlock);
	dAreaFree.subtract(areaToBlock);
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

void Zone::processModificators()
{
	for(auto & modificator : modificators)
	{
		try
		{
			modificator->run();
		}
		catch (const rmgException & e)
		{
			logGlobal->info("Zone %d, modificator %s - FAILED: %s", getId(), e.what());
			throw e;
		}
	}
	logGlobal->info("Zone %d filled successfully", getId());
}

Modificator::Modificator(Zone & zone, RmgMap & map, CMapGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void Modificator::setName(const std::string & n)
{
	name = n;
}

const std::string & Modificator::getName() const
{
	return name;
}

bool Modificator::isFinished() const
{
	return finished;
}

void Modificator::run()
{
	started = true;
	if(!finished)
	{
		for(auto * modificator : preceeders)
		{
			if(!modificator->started)
				modificator->run();
		}
		logGlobal->info("Modificator zone %d - %s - started", zone.getId(), getName());
		CStopWatch processTime;
		try
		{
			process();
		}
		catch(rmgException &e)
		{
			logGlobal->error("Modificator %s, exception: %s", getName(), e.what());
		}
#ifdef RMG_DUMP
		dump();
#endif
		finished = true;
		logGlobal->info("Modificator zone %d - %s - done (%d ms)", zone.getId(), getName(), processTime.getDiff());
	}
}

void Modificator::dependency(Modificator * modificator)
{
	if(modificator && modificator != this)
	{
		if(std::find(preceeders.begin(), preceeders.end(), modificator) == preceeders.end())
			preceeders.push_back(modificator);
	}
}

void Modificator::postfunction(Modificator * modificator)
{
	if(modificator && modificator != this)
	{
		if(std::find(modificator->preceeders.begin(), modificator->preceeders.end(), this) == modificator->preceeders.end())
			modificator->preceeders.push_back(this);
	}
}

void Modificator::dump()
{
	std::ofstream out(boost::to_string(boost::format("seed_%d_modzone_%d_%s.txt") % generator.getRandomSeed() % zone.getId() % getName()));
	auto & mapInstance = map.map();
	int levels = mapInstance.levels();
	int width =  mapInstance.width;
	int height = mapInstance.height;
	for(int z = 0; z < levels; z++)
	{
		for(int j=0; j<height; j++)
		{
			for(int i=0; i<width; i++)
			{
				out << dump(int3(i, j, z));
			}
			out << std::endl;
		}
		out << std::endl;
	}
	out << std::endl;
}

char Modificator::dump(const int3 & t)
{
	if(zone.freePaths().contains(t))
		return '.'; //free path
	if(zone.areaPossible().contains(t))
		return ' '; //possible
	if(zone.areaUsed().contains(t))
		return 'U'; //used
	if(zone.area().contains(t))
	{
		if(map.shouldBeBlocked(t))
			return '#'; //obstacle
		else
			return '^'; //visitable points?
	}
	return '?';
}

VCMI_LIB_NAMESPACE_END
