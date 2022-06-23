//
//  Zone.cpp
//  vcmi
//
//  Created by nordsoft on 22.06.2022.
//

#include "Zone.h"
#include "CMapGenerator.h"
#include "Functions.h"

Zone::Zone(CMapGenerator * Gen)
					: ZoneOptions(),
					townType(ETownType::NEUTRAL),
					terrainType(Terrain("grass")),
					gen(Gen)
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

void Zone::addTile (const int3 &Pos)
{
	tileinfo.insert(Pos);
}

void Zone::removeTile(const int3 & Pos)
{
	tileinfo.erase(Pos);
	possibleTiles.erase(Pos);
}

const std::set<int3> & Zone::getTileInfo() const
{
	return tileinfo;
}

const std::set<int3> & Zone::getPossibleTiles() const
{
	return possibleTiles;
}

void Zone::clearTiles()
{
	tileinfo.clear();
}

void Zone::initFreeTiles()
{
	vstd::copy_if(tileinfo, vstd::set_inserter(possibleTiles), [this](const int3 &tile) -> bool
	{
		return gen->isPossible(tile);
	});
	if(freePaths.empty())
	{
		addFreePath(pos); //zone must have at least one free tile where other paths go - for instance in the center
	}
}

const std::set<int3> & Zone::getFreePaths() const
{
	return freePaths;
}

void Zone::addFreePath(const int3 & p)
{
	gen->setOccupied(p, ETileType::FREE);
	freePaths.insert(p);
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
					if (!gen->isBlocked(pos))
					{
						if (gen->getZoneID(pos) == id)
						{
							if (gen->isPossible(pos))
							{
								gen->setOccupied (pos, ETileType::FREE);
								if (clearedTiles)
									clearedTiles->insert(pos);
								currentPos = pos;
								distance = static_cast<float>(currentPos.dist2dSQ (dst));
							}
							else if (gen->isFree(pos))
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
			gen->foreachDirectNeighbour (currentPos, processNeighbours);
		else
			gen->foreach_neighbour (currentPos,processNeighbours);
		
		int3 anotherPos(-1, -1, -1);
		
		if (!(result || distance < lastDistance)) //we do not advance, use more advanced pathfinding algorithm?
		{
			//try any nearby tiles, even if its not closer than current
			float lastDistance = 2 * distance; //start with significantly larger value
			
			auto processNeighbours2 = [this, &currentPos, dst, &lastDistance, &anotherPos, clearedTiles](int3 &pos)
			{
				if (currentPos.dist2dSQ(dst) < lastDistance) //try closest tiles from all surrounding unused tiles
				{
					if (gen->getZoneID(pos) == id)
					{
						if (gen->isPossible(pos))
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
				gen->foreachDirectNeighbour(currentPos, processNeighbours2);
			else
				gen->foreach_neighbour(currentPos, processNeighbours2);
			
			
			if (anotherPos.valid())
			{
				if (clearedTiles)
					clearedTiles->insert(anotherPos);
				gen->setOccupied(anotherPos, ETileType::FREE);
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
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm
	
	std::set<int3> closed;    // The set of nodes already evaluated.
	auto open = createPriorityQueue();    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;
	
	//int3 currentNode = src;
	
	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0.f;
	open.push(std::make_pair(src, 0.f));
	// Cost from start along best known path.
	// Estimated total cost from start to goal through y.
	
	while (!open.empty())
	{
		auto node = open.top();
		open.pop();
		int3 currentNode = node.first;
		
		closed.insert(currentNode);
		
		if (gen->isFree(currentNode)) //we reached free paths, stop
		{
			// Trace the path using the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].valid())
			{
				gen->setOccupied(backTracking, ETileType::FREE);
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			auto foo = [this, &open, &closed, &cameFrom, &currentNode, &distances](int3& pos) -> void
			{
				if (vstd::contains(closed, pos))
					return;
				
				//no paths through blocked or occupied tiles, stay within zone
				if (gen->isBlocked(pos) || gen->getZoneID(pos) != id)
					return;
				
				int distance = static_cast<int>(distances[currentNode]) + 1;
				int bestDistanceSoFar = std::numeric_limits<int>::max();
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = static_cast<int>(it->second);
				
				if (distance < bestDistanceSoFar)
				{
					cameFrom[pos] = currentNode;
					open.push(std::make_pair(pos, (float)distance));
					distances[pos] = static_cast<float>(distance);
				}
			};
			
			if (onlyStraight)
				gen->foreachDirectNeighbour(currentNode, foo);
			else
				gen->foreach_neighbour(currentNode, foo);
		}
		
	}
	for (auto tile : closed) //these tiles are sealed off and can't be connected anymore
	{
		if(gen->isPossible(tile))
			gen->setOccupied (tile, ETileType::BLOCKED);
		vstd::erase_if_present(possibleTiles, tile);
	}
	return false;
}

bool Zone::connectWithCenter(const int3 & src, bool onlyStraight, bool passThroughBlocked)
///connect current tile to any other free tile within zone
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm
	
	std::set<int3> closed;    // The set of nodes already evaluated.
	auto open = createPriorityQueue(); // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;
	
	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	open.push(std::make_pair(src, 0.f));
	// Cost from start along best known path.
	
	while (!open.empty())
	{
		auto node = open.top();
		open.pop();
		int3 currentNode = node.first;
		
		closed.insert(currentNode);
		
		if (currentNode == pos) //we reached center of the zone, stop
		{
			// Trace the path using the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].valid())
			{
				gen->setOccupied(backTracking, ETileType::FREE);
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			auto foo = [this, &open, &closed, &cameFrom, &currentNode, &distances, passThroughBlocked](int3& pos) -> void
			{
				if (vstd::contains(closed, pos))
					return;
				
				if (gen->getZoneID(pos) != id)
					return;
				
				float movementCost = 0;
				
				if (gen->isFree(pos))
					movementCost = 1;
				else if (gen->isPossible(pos))
					movementCost = 2;
				else if(passThroughBlocked && gen->shouldBeBlocked(pos))
					movementCost = 3;
				else
					return;
				
				float distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				int bestDistanceSoFar = std::numeric_limits<int>::max(); //FIXME: boost::limits
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = static_cast<int>(it->second);
				
				if (distance < bestDistanceSoFar)
				{
					cameFrom[pos] = currentNode;
					open.push(std::make_pair(pos, distance));
					distances[pos] = distance;
				}
			};
			
			if (onlyStraight)
				gen->foreachDirectNeighbour(currentNode, foo);
			else
				gen->foreach_neighbour(currentNode, foo);
		}
		
	}
	return false;
}
