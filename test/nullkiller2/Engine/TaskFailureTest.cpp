/*
 * TaskFailureTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/Nullkiller.h"

TEST(Nullkiller2_Engine_TaskFailure, triesNextTaskWhenAnotherCandidateIsAvailable)
{
	EXPECT_EQ(
		NK2AI::chooseTaskFailureAction(false, true, false),
		NK2AI::TaskFailureAction::TRY_NEXT_TASK);
}

TEST(Nullkiller2_Engine_TaskFailure, replansAfterPreviousProgressEvenWithRemainingTasks)
{
	EXPECT_EQ(
		NK2AI::chooseTaskFailureAction(true, true, false),
		NK2AI::TaskFailureAction::REPLAN);
}

TEST(Nullkiller2_Engine_TaskFailure, replansAfterPreviousProgress)
{
	EXPECT_EQ(
		NK2AI::chooseTaskFailureAction(true, false, false),
		NK2AI::TaskFailureAction::REPLAN);
}

TEST(Nullkiller2_Engine_TaskFailure, replansWhenAnotherHeroCanStillMove)
{
	EXPECT_EQ(
		NK2AI::chooseTaskFailureAction(false, false, true),
		NK2AI::TaskFailureAction::REPLAN);
}

TEST(Nullkiller2_Engine_TaskFailure, stopsWhenNoProgressOrAlternativeExists)
{
	EXPECT_EQ(
		NK2AI::chooseTaskFailureAction(false, false, false),
		NK2AI::TaskFailureAction::STOP_TURN);
}
