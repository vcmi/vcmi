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
class ObjectManager;
class ObjectTemplate;

std::set<int3> DLL_LINKAGE collectDistantTiles(const Zone & zone, float distance);

void DLL_LINKAGE createBorder(CMapGenerator & gen, const Zone & zone);

si32 DLL_LINKAGE getRandomTownType(const Zone & zone, CRandomGenerator & generator, bool matchUndergroundType = false);

void DLL_LINKAGE paintZoneTerrain(const Zone & zone, CMapGenerator & gen, const Terrain & terrainType);

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

bool DLL_LINKAGE placeMines(const Zone & zone, CMapGenerator & gen, ObjectManager & manager);

void DLL_LINKAGE initTownType(Zone & zone, CMapGenerator & gen, ObjectManager & manager);

void DLL_LINKAGE createObstacles1(const Zone & zone, CMapGenerator & gen);
void DLL_LINKAGE createObstacles2(const Zone & zone, CMapGenerator & gen, ObjectManager & manager);
bool canObstacleBePlacedHere(const CMapGenerator & gen, ObjectTemplate &temp, int3 &pos);
void placeSubterraneanGate(Zone & zone, ObjectManager & manager, int3 pos, si32 guardStrength);

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, const Terrain & terrain);
