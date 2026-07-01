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

void expectCastsDimensionDoor(const NK2AI::DimensionDoorExplorationCandidate & candidate)
{
	EXPECT_TRUE(NK2AI::evaluateDimensionDoorExplorationCandidate(candidate).accepted);
}

void expectDoesNotCastDimensionDoor(const NK2AI::DimensionDoorExplorationCandidate & candidate)
{
	EXPECT_FALSE(NK2AI::evaluateDimensionDoorExplorationCandidate(candidate).accepted);
}
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForHiddenDiscoveryWhenGuardsDoNotTrigger)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.tilesDiscovered, 5);
	EXPECT_FLOAT_EQ(evaluation.value, 100.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForPostLandingContinuation)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;
	candidate.continuationTilesDiscovered = 10;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.tilesDiscovered, 5);
	EXPECT_FLOAT_EQ(evaluation.value, 500.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForPostLandingContinuationWithoutImmediateDiscovery)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.continuationTilesDiscovered = 10;

	const auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.tilesDiscovered, 1);
	EXPECT_FLOAT_EQ(evaluation.value, 400.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsWhenPostLandingContinuationBeatsRawDiscovery)
{
	auto rawDiscovery = makeCandidate();
	rawDiscovery.visible = false;
	rawDiscovery.tilesDiscovered = 12;

	const auto rawEvaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(rawDiscovery);
	ASSERT_TRUE(rawEvaluation.accepted);

	auto continuation = makeCandidate();
	continuation.visible = false;
	continuation.tilesDiscovered = 6;
	continuation.continuationTilesDiscovered = 12;
	continuation.currentBestValue = rawEvaluation.value;

	auto continuationEvaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(continuation);

	EXPECT_TRUE(continuationEvaluation.accepted);
	EXPECT_GT(continuationEvaluation.value, rawEvaluation.value);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsWhenMultiCastChainBeatsRawDiscovery)
{
	auto rawDiscovery = makeCandidate();
	rawDiscovery.visible = false;
	rawDiscovery.tilesDiscovered = 12;

	const auto rawEvaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(rawDiscovery);
	ASSERT_TRUE(rawEvaluation.accepted);

	auto chain = makeCandidate();
	chain.visible = false;
	chain.tilesDiscovered = 6;
	chain.chainTilesDiscovered = 12;
	chain.currentBestValue = rawEvaluation.value;

	auto chainEvaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(chain);

	EXPECT_TRUE(chainEvaluation.accepted);
	EXPECT_EQ(chainEvaluation.tilesDiscovered, 18);
	EXPECT_GT(chainEvaluation.value, rawEvaluation.value);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForChainDiscoveryWithoutImmediateDiscovery)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.chainTilesDiscovered = 5;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_EQ(evaluation.tilesDiscovered, 5);
	EXPECT_FLOAT_EQ(evaluation.value, 100.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, doesNotCastForWalkingReachableLanding)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 20;
	candidate.chainTilesDiscovered = 20;
	candidate.strategicScore = 10.0f;
	candidate.reachableWithoutDimensionDoor = true;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, doesNotCastIntoHiddenTilesWhenDimensionDoorTriggersGuards)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;
	candidate.dimensionDoorTriggersGuards = true;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForVisibleStrategicTargetWithoutNewDiscovery)
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

TEST(Nullkiller2_Helpers_DimensionDoorExploration, doesNotCastForVisibleTileWithoutDiscoveryOrStrategicProgress)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 0;
	candidate.strategicScore = 0.0f;

	auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_FALSE(evaluation.accepted);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForSafeVisibleGuardedLanding)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 5;
	candidate.dimensionDoorTriggersGuards = true;
	candidate.guardedLandingDanger = 100;
	candidate.guardedLandingSafe = true;

	expectCastsDimensionDoor(candidate);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsForUnguardedVisibleLandingEvenWhenGuardRuleIsEnabled)
{
	auto candidate = makeCandidate();
	candidate.visible = true;
	candidate.tilesDiscovered = 5;
	candidate.dimensionDoorTriggersGuards = true;
	candidate.guardedLandingDanger = 0;
	candidate.guardedLandingSafe = false;

	expectCastsDimensionDoor(candidate);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, doesNotCastForUnsafeVisibleGuardedLanding)
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

TEST(Nullkiller2_Helpers_DimensionDoorExploration, doesNotCastWhenLandingDoesNotImproveCurrentBest)
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

TEST(Nullkiller2_Helpers_DimensionDoorExploration, doesNotCastWhenCurrentBestHasEqualValue)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;
	candidate.currentBestValue = 100.0f;

	expectDoesNotCastDimensionDoor(candidate);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsWhenCurrentBestHasLowerValue)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;
	candidate.currentBestValue = 99.0f;

	expectCastsDimensionDoor(candidate);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, castsWithClampedMovementCostWhenCastConsumesNoMovement)
{
	auto candidate = makeCandidate();
	candidate.visible = false;
	candidate.tilesDiscovered = 5;
	candidate.movementPointsTaken = 0;

	const auto evaluation = NK2AI::evaluateDimensionDoorExplorationCandidate(candidate);

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.value, 250.0f);
}

TEST(Nullkiller2_Helpers_DimensionDoorExploration, dimensionDoorReturnsToPlannerBeforeNeighbourExploration)
{
	EXPECT_FALSE(NK2AI::shouldExploreNeighbourAfterExplorationGoal(NK2AI::Goals::ADVENTURE_SPELL_CAST));
	EXPECT_TRUE(NK2AI::shouldExploreNeighbourAfterExplorationGoal(NK2AI::Goals::EXECUTE_HERO_CHAIN));
}
