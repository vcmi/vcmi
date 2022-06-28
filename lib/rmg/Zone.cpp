/*
 * Zone.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "Zone.h"
#include "RmgMap.h"
#include "Functions.h"
#include "TileInfo.h"
#include "../mapping/CMap.h"
#include "../CStopWatch.h"
#include "CMapGenerator.h"
#include "RmgPath.h"

Zone::Zone(RmgMap & map, CMapGenerator & generator)
					: ZoneOptions(),
					townType(ETownType::NEUTRAL),
					terrainType(Terrain("grass")),
					map(map),
					generator(generator)
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

const Terrain & Zone::getTerrainType() const
{
	return terrainType;
}

void Zone::setTerrainType(const Terrain & terrain)
{
	terrainType = terrain;
}

bool Zone::crunchPath(const int3 &src, const int3 &dst, bool onlyStraight, std::set<int3>* clearedTiles)
{
	/*
	 make shortest path with free tiles, reachning dst or closest already free tile. Avoid blocks.
	 do not leave zone border
	 */
	bool result = false;
	bool end = false;
	
	int3 currentPos = src;
	float distance = static_cast<float>(currentPos.dist2dSQ (dst));
	
	while (!end)
	{
		if (currentPos == dst)
		{
			result = true;
			break;
		}
		
		auto lastDistance = distance;
		
		auto processNeighbours = [this, &currentPos, dst, &distance, &result, &end, clearedTiles](int3 &pos)
		{
			if (!result) //not sure if lambda is worth it...
			{
				if (pos == dst)
				{
					result = true;
					end = true;
				}
				if (pos.dist2dSQ (dst) < distance)
				{
					if (!map.isBlocked(pos))
					{
						if (map.getZoneID(pos) == id)
						{
							if (map.isPossible(pos))
							{
								map.setOccupied (pos, ETileType::FREE);
								if(clearedTiles)
									clearedTiles->insert(pos);
								currentPos = pos;
								distance = static_cast<float>(currentPos.dist2dSQ (dst));
							}
							else if(map.isFree(pos))
							{
								end = true;
								result = true;
							}
						}
					}
				}
			}
		};
		
		if (onlyStraight)
			map.foreachDirectNeighbour (currentPos, processNeighbours);
		else
			map.foreach_neighbour (currentPos,processNeighbours);
		
		int3 anotherPos(-1, -1, -1);
		
		if (!(result || distance < lastDistance)) //we do not advance, use more advanced pathfinding algorithm?
		{
			//try any nearby tiles, even if its not closer than current
			float lastDistance = 2 * distance; //start with significantly larger value
			
			auto processNeighbours2 = [this, &currentPos, dst, &lastDistance, &anotherPos, clearedTiles](int3 &pos)
			{
				if (currentPos.dist2dSQ(dst) < lastDistance) //try closest tiles from all surrounding unused tiles
				{
					if (map.getZoneID(pos) == id)
					{
						if (map.isPossible(pos))
						{
							if (clearedTiles)
								clearedTiles->insert(pos);
							anotherPos = pos;
							lastDistance = static_cast<float>(currentPos.dist2dSQ(dst));
						}
					}
				}
			};
			if (onlyStraight)
				map.foreachDirectNeighbour(currentPos, processNeighbours2);
			else
				map.foreach_neighbour(currentPos, processNeighbours2);
			
			
			if (anotherPos.valid())
			{
				if (clearedTiles)
					clearedTiles->insert(anotherPos);
				//map.setOccupied(anotherPos, ETileType::FREE);
				currentPos = anotherPos;
			}
		}
		if (!(result || distance < lastDistance || anotherPos.valid()))
		{
			//FIXME: seemingly this condition is messed up, tells nothing
			//logGlobal->warn("No tile closer than %s found on path from %s to %s", currentPos, src , dst);
			break;
		}
	}
	
	return result;
}

bool Zone::connectPath(const rmg::Area & src, bool onlyStraight)
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
	
	auto area = dAreaPossible + dAreaFree;
	rmg::Path freePath(area);
	freePath.connect(dAreaFree);
	
	//connect to all pieces
	rmg::Area potentialPath;
	auto goals = connectedAreas(src);
	for(auto & goal : goals)
	{
		auto path = freePath.search(goal, onlyStraight, movementCost);
		if(path.getPathArea().empty())
			return false;
		
		potentialPath.unite(path.getPathArea());
		freePath.connect(path.getPathArea());
	}
	
	dAreaPossible.subtract(potentialPath);
	dAreaFree.unite(potentialPath);
	for(auto & t : potentialPath.getTiles())
		map.setOccupied(t, ETileType::FREE);
	return true;
}

bool Zone::connectPath(const int3 & src, bool onlyStraight)
///connect current tile to any other free tile within zone
{
	return connectPath(rmg::Area({src}), onlyStraight);
}

void Zone::fractalize()
{
	rmg::Area clearedTiles(dAreaFree);
	rmg::Area possibleTiles(dAreaPossible);
	rmg::Area tilesToIgnore; //will be erased in this iteration
	
	//the more treasure density, the greater distance between paths. Scaling is experimental.
	int totalDensity = 0;
	for(auto ti : treasureInfo)
		totalDensity += ti.density;
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
			
			for(auto tileToMakePath : tilesToMakePath)
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
	auto areas = connectedAreas(clearedTiles);
	for(auto & area : areas)
	{
		if(dAreaFree.overlap(area))
			continue; //already found
			
		rmg::Path path(dAreaPossible + dAreaFree);
		path.connect(dAreaFree);
		auto res = path.search(area, false);
		if(res.getPathArea().empty())
		{
			dAreaPossible.subtract(area);
			dAreaFree.subtract(area);
			for(auto & t : area.getTiles())
				map.setOccupied(t, ETileType::BLOCKED);
		}
		else
		{
			dAreaPossible.subtract(res.getPathArea());
			dAreaFree.unite(res.getPathArea());
			for(auto & t : res.getPathArea().getTiles())
				map.setOccupied(t, ETileType::FREE);
		}
	}
	
	//now block most distant tiles away from passages
	float blockDistance = minDistance * 0.25f;
	auto areaToBlock = dArea.getSubarea([this, blockDistance](const int3 & t)
	{
		float distance = static_cast<float>(dAreaFree.distanceSqr(t));
		return distance > blockDistance;
	});
	dAreaPossible.subtract(areaToBlock);
	dAreaFree.subtract(areaToBlock);
	for(auto & t : areaToBlock.getTiles())
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
		map.dump(false);
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
		CStopWatch processTime;
		process();
		dump();
		finished = true;
		logGlobal->info("Modificator zone %d - %s - done (%d ms)", zone.getId(), getName(), processTime.getDiff());
	}
}

void Modificator::dependency(Modificator * modificator)
{
	if(modificator && modificator != this)
	{
		preceeders.insert(modificator);
	}
}

void Modificator::postfunction(Modificator * modificator)
{
	if(modificator && modificator != this)
	{
		modificator->preceeders.insert(this);
	}
}

void Modificator::dump()
{
	static int order = 1;
	std::ofstream out(boost::to_string(boost::format("n%s_modzone_%d_%s.txt") % boost::io::group(std::setfill('0'), std::setw(3), order++) % zone.getId() % getName()));
	auto & mapInstance = map.map();
	int levels = mapInstance.twoLevel ? 2 : 1;
	int width =  mapInstance.width;
	int height = mapInstance.height;
	for (int k = 0; k < levels; k++)
	{
		for(int j=0; j<height; j++)
		{
			for (int i=0; i<width; i++)
			{
				out << dump(int3(i, j, k));
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
