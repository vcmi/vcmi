/*
 * PriorityEvaluatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/PriorityEvaluator.h"

TEST(Nullkiller2_Engine_PriorityEvaluator, enemyTownConquestBeatsOrdinaryHeroHunting)
{
	EXPECT_GT(NK2AI::evaluateEnemyTownConquestValue(0.8f, 3), 2.0f * 1.5f)
		<< "visible enemy towns should clearly outrank the capped strategic value of a loose enemy hero";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, lastVisibleEnemyTownGetsEliminationPressure)
{
	EXPECT_GT(
		NK2AI::evaluateEnemyTownConquestValue(1.0f, 1),
		NK2AI::evaluateEnemyTownConquestValue(1.5f, 3))
		<< "taking the last visible enemy town should outrank a normal enemy capital capture";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, nonEnemyTownKeepsBaseConquestValue)
{
	EXPECT_FLOAT_EQ(
		NK2AI::evaluateEnemyTownConquestValue(1.2f, 0),
		1.2f)
		<< "only visible enemy towns should get extra conquest pressure";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, enemyTownConquestCanRelaxArmyLossLimit)
{
	EXPECT_GT(
		NK2AI::evaluateMaxArmyLossForConquest(0.35f, 5.25f, true),
		0.39f)
		<< "enemy town conquest should not be rejected by the ordinary creeping loss cap";
}

TEST(Nullkiller2_Engine_PriorityEvaluator, ordinaryTargetsKeepArmyLossLimit)
{
	EXPECT_FLOAT_EQ(
		NK2AI::evaluateMaxArmyLossForConquest(0.35f, 5.25f, false),
		0.35f)
		<< "the relaxed loss cap is only for enemy-town conquest";
}
