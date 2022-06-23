//
//  RoadPlacer.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "RoadPlacer.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "RmgMap.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"

RoadPlacer::RoadPlacer(Zone & zone, RmgMap & map, CRandomGenerator & generator) : zone(zone), map(map), generator(generator)
{
}

void RoadPlacer::process()
{
	drawRoads();
}

bool RoadPlacer::createRoad(const int3 & src, const int3 & dst)
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm
	
	std::set<int3> closed;    // The set of nodes already evaluated.
	auto pq = createPriorityQueue();    // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;
	
	map.setRoad(src, ROAD_NAMES[0]); //just in case zone guard already has road under it. Road under nodes will be added at very end
	
	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	pq.push(std::make_pair(src, 0.f));
	distances[src] = 0.f;
	// Cost from start along best known path.
	
	while (!pq.empty())
	{
		auto node = pq.top();
		pq.pop(); //remove top element
		int3 currentNode = node.first;
		closed.insert (currentNode);
		auto currentTile = &map.map().getTile(currentNode);
		
		if (currentNode == dst || map.isRoad(currentNode))
		{
			// The goal node was reached. Trace the path using
			// the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].valid())
			{
				// add node to path
				roads.insert(backTracking);
				map.setRoad(backTracking, ROAD_NAMES[1]);
				//gen.setRoad(backTracking, gen.getConfig().defaultRoadType);
				//logGlobal->trace("Setting road at tile %s", backTracking);
				// do the same for the predecessor
				backTracking = cameFrom[backTracking];
			}
			return true;
		}
		else
		{
			bool directNeighbourFound = false;
			float movementCost = 1;
			
			auto foo = [this, &pq, &distances, &closed, &cameFrom, &currentNode, &currentTile, &node, &dst, &directNeighbourFound, &movementCost](int3& pos) -> void
			{
				if (vstd::contains(closed, pos)) //we already visited that node
					return;
				float distance = node.second + movementCost;
				float bestDistanceSoFar = std::numeric_limits<float>::max();
				auto it = distances.find(pos);
				if (it != distances.end())
					bestDistanceSoFar = it->second;
				
				if (distance < bestDistanceSoFar)
				{
					auto tile = &map.map().getTile(pos);
					bool canMoveBetween = map.map().canMoveBetween(currentNode, pos);
					
					if((map.isFree(pos) && map.isFree(currentNode)) //empty path
						|| ((tile->visitable || currentTile->visitable) && canMoveBetween) //moving from or to visitable object
						|| pos == dst) //we already compledted the path
					{
						if (map.getZoneID(pos) == zone.getId() || pos == dst) //otherwise guard position may appear already connected to other zone.
						{
							cameFrom[pos] = currentNode;
							distances[pos] = distance;
							pq.push(std::make_pair(pos, distance));
							directNeighbourFound = true;
						}
					}
				}
			};
			
			map.foreachDirectNeighbour(currentNode, foo); // roads cannot be rendered correctly for diagonal directions
			if (!directNeighbourFound)
			{
				movementCost = 2.1f; //moving diagonally is penalized over moving two tiles straight
				map.foreachDiagonalNeighbour(currentNode, foo);
			}
		}
		
	}
	logGlobal->warn("Failed to create road from %s to %s", src.toString(), dst.toString());
	return false;
	
}

void RoadPlacer::drawRoads()
{
	std::vector<int3> tiles;
	for (auto tile : roads)
	{
		if(map.isOnMap(tile))
			tiles.push_back (tile);
	}
	for (auto tile : roadNodes)
	{
		if (map.getZoneID(tile) == zone.getId()) //mark roads for our nodes, but not for zone guards in other zones
			tiles.push_back(tile);
	}
	
	map.getEditManager()->getTerrainSelection().setSelection(tiles);
	//map.getEditManager()->drawRoad(gen.getConfig().defaultRoadType, &generator);
	map.getEditManager()->drawRoad(ROAD_NAMES[1], &generator);
}

void RoadPlacer::addRoadNode(const int3& node)
{
	roadNodes.insert(node);
}

void RoadPlacer::connectRoads()
{
	logGlobal->debug("Started building roads");
	
	std::set<int3> roadNodesCopy(roadNodes);
	std::set<int3> processed;
	
	while(!roadNodesCopy.empty())
	{
		int3 node = *roadNodesCopy.begin();
		roadNodesCopy.erase(node);
		int3 cross(-1, -1, -1);
		
		auto comparator = [=](int3 lhs, int3 rhs) { return node.dist2dSQ(lhs)  < node.dist2dSQ(rhs); };
		
		if(processed.size()) //connect with already existing network
		{
			cross = *boost::range::min_element(processed, comparator); //find another remaining node
		}
		else if(roadNodesCopy.size()) //connect with any other unconnected node
		{
			cross = *boost::range::min_element(roadNodesCopy, comparator); //find another remaining node
		}
		else //no other nodes left, for example single road node in this zone
			break;
		
		logGlobal->debug("Building road from %s to %s", node.toString(), cross.toString());
		if(createRoad(node, cross))
		{
			processed.insert(cross); //don't draw road starting at end point which is already connected
			vstd::erase_if_present(roadNodesCopy, cross);
		}
		
		processed.insert(node);
	}
	
	drawRoads();
	
	logGlobal->debug("Finished building roads");
}
