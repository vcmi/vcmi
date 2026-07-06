/*
 * ExploreNeighbourTileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Goals/ExploreNeighbourTile.h"

namespace
{
NK2AI::NeighbourExplorationCandidate makeAcceptedCandidate()
{
	NK2AI::NeighbourExplorationCandidate candidate;
	candidate.sameDay = true;
	candidate.accessible = true;
	candidate.safe = true;
	candidate.tilesDiscovered = 3;
	candidate.movementCost = 0.5f;
	return candidate;
}
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, acceptsSafeSameDayDiscovery)
{
	const auto evaluation = NK2AI::evaluateNeighbourExplorationCandidate(makeAcceptedCandidate());

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 18.0f);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, discountsNormalMovementCost)
{
	auto cheaperCandidate = makeAcceptedCandidate();
	auto expensiveCandidate = makeAcceptedCandidate();
	expensiveCandidate.movementCost = 1.5f;

	EXPECT_GT(
		NK2AI::evaluateNeighbourExplorationCandidate(cheaperCandidate).value,
		NK2AI::evaluateNeighbourExplorationCandidate(expensiveCandidate).value)
		<< "equal discovery should prefer the neighbour that leaves more movement";
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, clampsTinyMovementCost)
{
	auto candidate = makeAcceptedCandidate();
	candidate.movementCost = 0.05f;

	const auto evaluation = NK2AI::evaluateNeighbourExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 90.0f);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsNextDayMove)
{
	auto candidate = makeAcceptedCandidate();
	candidate.sameDay = false;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsUnsafeDestination)
{
	auto candidate = makeAcceptedCandidate();
	candidate.safe = false;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}

TEST(Nullkiller2_Goals_ExploreNeighbourTile, rejectsMoveWithNoDiscovery)
{
	auto candidate = makeAcceptedCandidate();
	candidate.tilesDiscovered = 0;

	EXPECT_FALSE(NK2AI::evaluateNeighbourExplorationCandidate(candidate).accepted);
}
