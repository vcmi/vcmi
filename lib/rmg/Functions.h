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

rmg::Tileset collectDistantTiles(const Zone & zone, int distance);

void createBorder(RmgMap & gen, Zone & zone);

void paintZoneTerrain(const Zone & zone, CRandomGenerator & generator, RmgMap & map, TTerrainId terrainType);

void initTerrainType(Zone & zone, CMapGenerator & gen);

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, TTerrainId terrain);

