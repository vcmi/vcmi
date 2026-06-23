/*
 * ExplorationHelperTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Helpers/ExplorationHelper.h"

namespace
{
NK2AI::DimensionDoorExplorationCandidate makeCandidate()
{
	NK2AI::DimensionDoorExplorationCandidate candidate;
	candidate.movementPointsRemaining = 1000;
	candidate.movementPointsLimit = 1000;
	candidate.movementPointsTaken = 250;
	return candidate;
}
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, acceptsHiddenDiscoveryWhenGuardsDoNotTrigger)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.tilesDiscovered, 5);
	EXPECT_FLOAT_EQ(evaluation.value, 100.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, rejectsHiddenLandingWhenDimensionDoorTriggersGuards)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;
	candidate.dimensionDoorTriggersGuards = true;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, acceptsVisibleStrategicTargetWithoutNewDiscovery)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 0;
	candidate.strategicScore = 4.0f;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.tilesDiscovered, 1);
	EXPECT_FLOAT_EQ(evaluation.value, 400.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, rejectsVisibleTileWithoutDiscoveryOrStrategicProgress)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 0;
	candidate.strategicScore = 0.0f;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, rejectsUnsafeVisibleGuardedLanding)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 5;
	candidate.dimensionDoorTriggersGuards = true;
	candidate.guardedLandingDanger = 100;
	candidate.guardedLandingSafe = false;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, requiresBetterValueThanCurrentBest)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.strategicScore = 1.0f;
	candidate.currentBestValue = 100.0f;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);

	candidate.currentBestValue = 99.0f;
	evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 100.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, dimensionDoorReturnsToPlannerBeforeNeighbourExploration)
{
	EXPECT_FALSE(NK2AI::shouldExploreNeighbourAfterExplorationGoal(NK2AI::Goals::ADVENTURE_SPELL_CAST));
	EXPECT_TRUE(NK2AI::shouldExploreNeighbourAfterExplorationGoal(NK2AI::Goals::EXECUTE_HERO_CHAIN));
}
