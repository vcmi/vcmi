//
//  Zone.cpp
//  vcmi
//
//  Created by nordsoft on 22.06.2022.
//

#include "Zone.h"
#include "RmgMap.h"
#include "Functions.h"
#include "TileInfo.h"
#include "../mapping/CMap.h"
#include "../CRandomGenerator.h"
#include "CRmgPath.h"

Zone::Zone(RmgMap & map, CRandomGenerator & generator)
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

const Rmg::Area & Zone::getArea() const
{
	return dArea;
}

Rmg::Area & Zone::area()
{
	return dArea;
}

Rmg::Area & Zone::areaPossible()
{
	return dAreaPossible;
}

Rmg::Area & Zone::areaBlocked()
{
	return dAreaPossible;
}

void Zone::clearTiles()
{
	dArea.clear();
	dAreaPossible.clear();
	dAreaBlocked.clear();
	dAreaFree.clear();
}

void Zone::initFreeTiles()
{
	Rmg::Tileset possibleTiles;
	vstd::copy_if(dArea.getTiles(), vstd::set_inserter(possibleTiles), [this](const int3 &tile) -> bool
	{
		return map.isPossible(tile);
	});
	dAreaPossible.assign(possibleTiles);
	
	if(dAreaFree.empty())
	{
		dAreaPossible.erase(pos);
		dAreaBlocked.erase(pos);
		dAreaFree.add(pos); //zone must have at least one free tile where other paths go - for instance in the center
	}
}

Rmg::Area & Zone::freePaths()
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

bool Zone::connectPath(const int3 & src, bool onlyStraight)
///connect current tile to any other free tile within zone
{
	Rmg::Path freePath(dAreaPossible + dAreaFree);
	freePath.connect(dAreaFree);
	auto path = freePath.search(src, onlyStraight);
	if(path.getPathArea().empty())
		return false;
	
	dAreaPossible.subtract(path.getPathArea());
	dAreaBlocked.subtract(path.getPathArea());
	dAreaFree.unite(path.getPathArea());
	for(auto & t : path.getPathArea().getTiles())
		map.setOccupied(t, ETileType::FREE);
	return true;
}

bool Zone::connectWithCenter(const int3 & src, bool onlyStraight, bool passThroughBlocked)
///connect current tile to any other free tile within zone
{
	auto movementCost = [this](const int3 & s, const int3 & d)
	{
		if(map.isFree(pos))
			return 1;
		else if (map.isPossible(pos))
			return 2;
		return 3;
	};
	auto area = dAreaPossible + dAreaFree;
	if(passThroughBlocked)
		area.unite(dAreaBlocked);
	Rmg::Path freePath(area, getPos());
	auto path = freePath.search(src, onlyStraight, movementCost);
	if(path.getPathArea().empty())
		return false;
	
	dAreaPossible.subtract(path.getPathArea());
	dAreaBlocked.subtract(path.getPathArea());
	dAreaFree.unite(path.getPathArea());
	for(auto & t : path.getPathArea().getTiles())
		map.setOccupied(t, ETileType::FREE);
	return true;
}

void Zone::addToConnectLater(const int3 & src)
{
	dTilesToConnectLater.add(src);
}

void Zone::connectLater()
{
	auto movementCost = [this](const int3 & s, const int3 & d)
	{
		if(map.isFree(pos))
			return 1;
		else if (map.isPossible(pos))
			return 2;
		return 3;
	};
	auto area = dAreaPossible + dAreaFree;
	Rmg::Path freePath(area, getPos());
	auto path = freePath.search(dTilesToConnectLater, true, movementCost);
	if(path.getPathArea().empty())
		logGlobal->error("Failed to connect node with center of the zone");
	
	dAreaPossible.subtract(path.getPathArea());
	dAreaBlocked.subtract(path.getPathArea());
	dAreaFree.unite(path.getPathArea());
	for(auto & t : path.getPathArea().getTiles())
		map.setOccupied(t, ETileType::FREE);
}

void Zone::fractalize()
{
	Rmg::Area clearedTiles(dAreaFree);
	Rmg::Area possibleTiles(dAreaPossible);
	Rmg::Area tilesToIgnore; //will be erased in this iteration
	
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
			std::vector<int3> tilesToMakePath(possibleTiles.getTiles().begin(), possibleTiles.getTiles().end());
			RandomGeneratorUtil::randomShuffle(tilesToMakePath, generator);
			
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
		Rmg::Path path(dAreaPossible + dAreaFree);
		path.connect(dAreaFree);
		path.search(area, false);
		if(path.getPathArea().empty())
		{
			dAreaBlocked.unite(area);
			dAreaPossible.subtract(area);
			dAreaFree.subtract(area);
			for(auto & t : area.getTiles())
				map.setOccupied(t, ETileType::BLOCKED);
		}
		else
		{
			dAreaPossible.subtract(path.getPathArea());
			dAreaBlocked.subtract(path.getPathArea());
			dAreaFree.unite(path.getPathArea());
			for(auto & t : path.getPathArea().getTiles())
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
	dAreaBlocked.subtract(areaToBlock);
	for(auto & t : areaToBlock.getTiles())
		map.setOccupied(t, ETileType::BLOCKED);
	
#define PRINT_FRACTALIZED_MAP false
	if (PRINT_FRACTALIZED_MAP) //enable to debug
	{
		std::ofstream out(boost::to_string(boost::format("zone_%d.txt") % id));
		int levels = map.map().twoLevel ? 2 : 1;
		int width =  map.map().width;
		int height = map.map().height;
		for (int k = 0; k < levels; k++)
		{
			for(int j=0; j<height; j++)
			{
				for (int i=0; i<width; i++)
				{
					char t = '?';
					switch (map.getTile(int3(i, j, k)).getTileType())
					{
						case ETileType::FREE:
							t = ' '; break;
						case ETileType::BLOCKED:
							t = '#'; break;
						case ETileType::POSSIBLE:
							t = '-'; break;
						case ETileType::USED:
							t = 'O'; break;
					}
					
					out << t;
				}
				out << std::endl;
			}
			out << std::endl;
		}
		out << std::endl;
	}
}
