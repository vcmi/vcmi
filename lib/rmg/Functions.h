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
#include <functional>
#include <memory>

class RmgMap;
class ObjectManager;
class ObjectTemplate;
class CMapGenerator;
class CCreature;
struct ObjectInfo;
class CGObjectInstance;

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
rmg::BezierCurve chooseCurve(const Zone & zone, const int3 & src, const rmg::DistanceMap & borderDistance, vstd::RNG & rng);

/// `roads` with dead-end spurs stripped; tiles in `keep` are never removed
rmg::Area pruneRoadDeadEnds(const rmg::Area & roads, const rmg::Tileset & keep);

/// `roads` without 2x2 squares, which are drawn as small rings. Each square loses one tile that is not
/// in `keep` and whose removal keeps the network connected, preferring the one that leaves a diagonal road.
rmg::Area removeRoadSquares(const rmg::Area & roads, const rmg::Tileset & keep);

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

/// Whether candidate can be added to an existing treasure pile
bool canJoinTreasurePile(const ObjectInfo & candidate, const std::vector<ObjectInfo *> & pile);

/// Whether every tile of footprint is at least minDistance (squared tile distance, as in the RMG distance map) away from placed
bool footprintKeepsDistance(const rmg::Area & footprint, const rmg::Area & placed, float minDistance);

/// Spell scroll map object holding `spell`.
std::shared_ptr<CGObjectInstance> createSpellScroll(RmgMap & map, SpellID spell);

/// The tile where IBoatGenerator::bestLocation() will put the boat:
/// the first origin + offset, in offsets order, that isWater accepts; int3(-1,-1,-1) if none.
DLL_LINKAGE int3 predictBoatTile(const std::vector<int3> & offsets, const int3 & origin, const std::function<bool(const int3 &)> & isWater);
