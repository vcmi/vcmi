/*
 * Functions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

rmg::Tileset collectDistantTiles(const Zone & zone, float distance);

void createBorder(RmgMap & gen, const Zone & zone);

void paintZoneTerrain(const Zone & zone, CRandomGenerator & generator, RmgMap & map, const Terrain & terrainType);

void initTerrainType(Zone & zone, CMapGenerator & gen);

void createObstacles1(const Zone & zone, RmgMap & map, CRandomGenerator & generator);
void createObstacles2(const Zone & zone, RmgMap & map, CRandomGenerator & generator, ObjectManager & manager);
bool canObstacleBePlacedHere(const RmgMap & gen, ObjectTemplate &temp, int3 &pos);

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, const Terrain & terrain);

void createObstaclesCommon1(RmgMap & map, CRandomGenerator & generator);
void createObstaclesCommon2(RmgMap & map, CRandomGenerator & generator);
