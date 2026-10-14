/*
 * ExplorationBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Behaviors/ExplorationBehavior.h"

TEST(Nullkiller2_Behaviors_ExplorationBehavior, acceptsAvailableBoatWithHiddenTiles)
{
	NK2AI::BoatExplorationCandidate candidate;
	candidate.available = true;
	candidate.hiddenTilesDiscovered = 12;

	const auto evaluation = NK2AI::evaluateBoatExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.explorationValue, 12);
}

TEST(Nullkiller2_Behaviors_ExplorationBehavior, rejectsUnavailableBoat)
{
	NK2AI::BoatExplorationCandidate candidate;
	candidate.available = false;
	candidate.hiddenTilesDiscovered = 12;

	EXPECT_FALSE(NK2AI::evaluateBoatExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_ExplorationBehavior, rejectsBoatWithoutHiddenTiles)
{
	NK2AI::BoatExplorationCandidate candidate;
	candidate.available = true;
	candidate.hiddenTilesDiscovered = 0;

	EXPECT_FALSE(NK2AI::evaluateBoatExplorationCandidate(candidate).accepted);
}
