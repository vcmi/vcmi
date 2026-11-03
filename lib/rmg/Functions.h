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
class CCreature;

class rmgException : public std::exception
{
	std::string msg;
public:
	explicit rmgException(const std::string& _Message) : msg(_Message)
	{
	}

	const char *what() const noexcept override
	{
		return msg.c_str();
	}
};

void replaceWithCurvedPath(rmg::Path & path, Zone & zone, const int3 & src, bool onlyStraight = true);

rmg::Tileset collectDistantTiles(const Zone & zone, int distance);

int chooseRandomAppearance(vstd::RNG & generator, si32 ObjID, TerrainId terrain);

/// Multiplier for the value of a dwelling of given faction, increases with the number of zones of that faction
float getNativeDwellingValueModifier(RmgMap & map, FactionID faction);

/// Value of a dwelling producing given creature, placed in a zone of its own faction, as in SoD
ui32 getNativeDwellingValue(RmgMap & map, const CCreature * creature);

/// Value of a random dwelling of given level (0-based) that becomes a dwelling of given faction, 0 if it can't be determined
ui32 getRandomDwellingValue(RmgMap & map, FactionID faction, int level);
