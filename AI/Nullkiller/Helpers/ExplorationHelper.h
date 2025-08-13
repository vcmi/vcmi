/*
* ExplorationHelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../AIUtility.h"

#include "../../../lib/GameConstants.h"
#include "../../../lib/GameLibrary.h"
#include "../Goals/AbstractGoal.h"

namespace NKAI
{

class ExplorationHelper
{
private:
	const CGHeroInstance * hero;
	int sightRadius;
	float bestValue;
	Goals::TSubgoal bestGoal;
	int3 bestTile;
	int bestTilesDiscovered;
	const Nullkiller * ai;
	CCallback * cbp;
	const TeamState * ts;
	int3 ourPos;
	bool allowDeadEndCancellation;
	bool useCPathfinderAccessibility;

public:
	ExplorationHelper(const CGHeroInstance * hero, const Nullkiller * ai, bool useCPathfinderAccessibility = false);
	Goals::TSubgoal makeComposition() const;
	bool scanSector(int scanRadius);
	bool scanMap();
	int howManyTilesWillBeDiscovered(const int3 & pos) const;

private:
	void scanTile(const int3 & tile);
	bool hasReachableNeighbor(const int3 & pos) const;
};

}
