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

VCMI_LIB_NAMESPACE_BEGIN
class CSpell;
class DimensionDoorEffect;
VCMI_LIB_NAMESPACE_END

namespace NK2AI
{

struct DimensionDoorExplorationCandidate
{
	bool visible = false;
	int tilesDiscovered = 0;
	float strategicScore = 0.0f;
	bool dimensionDoorTriggersGuards = false;
	uint64_t guardedLandingDanger = 0;
	bool guardedLandingSafe = true;
	int movementPointsRemaining = 0;
	int movementPointsLimit = 1;
	int movementPointsTaken = 0;
	float currentBestValue = 0.0f;
};

struct DimensionDoorExplorationEvaluation
{
	bool accepted = false;
	float value = 0.0f;
	int tilesDiscovered = 0;
};

DimensionDoorExplorationEvaluation evaluateDimensionDoorExplorationCandidate(
	const DimensionDoorExplorationCandidate & candidate);

bool shouldExploreNeighbourAfterExplorationGoal(Goals::EGoals goalType);

class ExplorationHelper
{
private:
	const CGHeroInstance * hero;
	int sightRadius;
	float bestValue;
	Goals::TSubgoal bestGoal;
	int3 bestTile;
	int bestTilesDiscovered;
	const Nullkiller * aiNk;
	CCallback * cc;
	const TeamState * ts;
	int3 ourPos;
	bool allowDeadEndCancellation;
	bool useCPathfinderAccessibility;

public:
	ExplorationHelper(const CGHeroInstance * hero, const Nullkiller * aiNk, bool useCPathfinderAccessibility = false);
	Goals::TSubgoal makeComposition() const;
	bool canUseDimensionDoor() const;
	bool scanSector(int scanRadius);
	bool scanMap();
	bool considerDimensionDoorExplorationTargets();
	int howManyTilesWillBeDiscovered(const int3 & pos) const;

private:
	void scanTile(const int3 & tile);
	void scanDimensionDoorTile(const CSpell * spell, const DimensionDoorEffect * effect, const int3 & tile);
	float getDimensionDoorStrategicScore(const int3 & tile) const;
	bool hasReachableNeighbor(const int3 & pos) const;
};

}
