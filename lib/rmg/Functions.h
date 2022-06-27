//
//  Functions.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once

#include "Zone.h"
#include <boost/heap/priority_queue.hpp> //A*

class RmgMap;
class ObjectManager;
class ObjectTemplate;
class CMapGenerator;

class rmgException : public std::exception
{
	std::string msg;
public:
	explicit rmgException(const std::string& _Message) : msg(_Message)
	{
	}
	
	virtual ~rmgException() throw ()
	{
	};
	
	const char *what() const throw () override
	{
		return msg.c_str();
	}
};

std::set<int3> DLL_LINKAGE collectDistantTiles(const Zone & zone, float distance);

void DLL_LINKAGE createBorder(RmgMap & gen, const Zone & zone);

void DLL_LINKAGE paintZoneTerrain(const Zone & zone, CRandomGenerator & generator, RmgMap & map, const Terrain & terrainType);

void DLL_LINKAGE initTerrainType(Zone & zone, CMapGenerator & gen);

void DLL_LINKAGE createObstacles1(const Zone & zone, RmgMap & map, CRandomGenerator & generator);
void DLL_LINKAGE createObstacles2(const Zone & zone, RmgMap & map, CRandomGenerator & generator, ObjectManager & manager);
bool canObstacleBePlacedHere(const RmgMap & gen, ObjectTemplate &temp, int3 &pos);

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, const Terrain & terrain);

bool processZone(Zone & zone, CMapGenerator & gen, RmgMap & map);

void createObstaclesCommon1(RmgMap & map, CRandomGenerator & generator);
void createObstaclesCommon2(RmgMap & map, CRandomGenerator & generator);
