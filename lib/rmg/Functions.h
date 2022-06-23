//
//  Functions.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once

#include "Zone.h"
#include <boost/heap/priority_queue.hpp> //A*

class CMapGenerator;

std::set<int3> collectDistantTiles(const Zone & zone, float distance);

void createBorder(CMapGenerator & gen, const Zone & zone);

si32 getRandomTownType(const Zone & zone, CRandomGenerator & generator, bool matchUndergroundType);

void paintZoneTerrain(const Zone & zone, CMapGenerator & gen, const Terrain & terrainType);

//A* priority queue
typedef std::pair<int3, float> TDistance;
struct NodeComparer
{
	bool operator()(const TDistance & lhs, const TDistance & rhs) const
	{
		return (rhs.second < lhs.second);
	}
};
boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue();
