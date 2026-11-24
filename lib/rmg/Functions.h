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

/// Creature of a random dwelling of given level (0-based) that becomes a dwelling of given faction, nullptr if there is none
const CCreature * getRandomDwellingCreature(FactionID faction, int level);

/// Value of a random dwelling of given level (0-based) that becomes a dwelling of given faction, 0 if it can't be determined
ui32 getRandomDwellingValue(RmgMap & map, FactionID faction, int level);

/// Levels that get their own water zone, ascending
std::vector<int> waterZoneLevels(EWaterContent::EWaterContent water,
	const std::vector<MapLayerId> & levelLayers, const std::set<int> & landZoneLevels);

/// A creature allowed to guard in a zone, with the guard strengths it stands for (exclusive bounds)
struct GuardCandidate
{
	CreatureID id;
	si32 aiValue;
	si32 minStrength; // aiValue * (advMapAmountMin + advMapAmountMax) / 2
	si32 maxStrength; // aiValue * 100
};

/// Guard creature for a strength that no candidate's window contains: the candidate whose window lies
/// nearest to the strength, the first one in order on a tie, Peasant if there are no candidates
CreatureID chooseFallbackGuard(si32 strength, const std::vector<GuardCandidate> & candidates);

/// Stack size of a guard of given strength, never below 1
si32 guardStackSize(si32 strength, si32 aiValue);
