/*
 * EscapeBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Behaviors/EscapeBehavior.h"
#include "AI/Nullkiller2/Engine/PriorityEvaluator.h"

namespace
{
NK2AI::EscapePathCandidate makeAcceptedCandidate()
{
	NK2AI::EscapePathCandidate candidate;
	candidate.currentTileThreatensHero = true;
	candidate.sameDay = true;
	candidate.singleHeroPath = true;
	candidate.destinationSafe = true;
	candidate.destinationIsSafer = true;
	candidate.threatReduction = 100.0f;
	candidate.movementCost = 0.5f;
	return candidate;
}
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, acceptsSafeSameDayThreatReduction)
{
	const auto evaluation = NK2AI::evaluateEscapePathCandidate(makeAcceptedCandidate());

	EXPECT_TRUE(evaluation.accepted);
	EXPECT_FLOAT_EQ(evaluation.score, 200.0f);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, rejectsWhenCurrentTileIsNotThreatened)
{
	auto candidate = makeAcceptedCandidate();
	candidate.currentTileThreatensHero = false;

	EXPECT_FALSE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, rejectsUnsafeDestination)
{
	auto candidate = makeAcceptedCandidate();
	candidate.destinationSafe = false;

	EXPECT_FALSE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, rejectsDestinationWithNoThreatReduction)
{
	auto candidate = makeAcceptedCandidate();
	candidate.destinationIsSafer = false;

	EXPECT_FALSE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, acceptsDimensionDoorPathWhenItIsOtherwiseValid)
{
	auto candidate = makeAcceptedCandidate();
	candidate.usesDimensionDoor = true;

	EXPECT_TRUE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, doesNotUseDimensionDoorForNextDayEscape)
{
	auto candidate = makeAcceptedCandidate();
	candidate.usesDimensionDoor = true;
	candidate.sameDay = false;

	EXPECT_FALSE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, doesNotUseDimensionDoorWhenLandingIsUnsafe)
{
	auto candidate = makeAcceptedCandidate();
	candidate.usesDimensionDoor = true;
	candidate.destinationSafe = false;

	EXPECT_FALSE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, doesNotUseDimensionDoorWhenItDoesNotReduceThreat)
{
	auto candidate = makeAcceptedCandidate();
	candidate.usesDimensionDoor = true;
	candidate.destinationIsSafer = false;

	EXPECT_FALSE(NK2AI::evaluateEscapePathCandidate(candidate).accepted);
}

TEST(Nullkiller2_Behaviors_EscapeBehavior, escapePriorityRunsBeforeExploration)
{
	EXPECT_LT(
		NK2AI::PriorityEvaluator::PriorityTier::KILL,
		NK2AI::PriorityEvaluator::PriorityTier::ESCAPE);
	EXPECT_LT(
		NK2AI::PriorityEvaluator::PriorityTier::ESCAPE,
		NK2AI::PriorityEvaluator::PriorityTier::EXPLORE_AND_GATHER);
}
