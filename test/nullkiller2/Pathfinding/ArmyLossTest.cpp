/*
 * ArmyLossTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIUtility.h"
#include "AI/Nullkiller2/Pathfinding/AINodeStorage.h"

#include <limits>

TEST(Nullkiller2_Pathfinding_ArmyLoss, normalizesInvalidHeroStrength)
{
	EXPECT_DOUBLE_EQ(NK2AI::normalizeHeroStrength(0.0), 1.0);
	EXPECT_DOUBLE_EQ(NK2AI::normalizeHeroStrength(-1.0), 1.0);
	EXPECT_DOUBLE_EQ(
		NK2AI::normalizeHeroStrength(std::numeric_limits<double>::quiet_NaN()),
		1.0);
	EXPECT_DOUBLE_EQ(
		NK2AI::normalizeHeroStrength(std::numeric_limits<double>::infinity()),
		1.0);
	EXPECT_DOUBLE_EQ(NK2AI::normalizeHeroStrength(1.25), 1.25);
}

TEST(Nullkiller2_Pathfinding_ArmyLoss, usesNeutralStrengthForZeroStrength)
{
	EXPECT_EQ(NK2AI::evaluateArmyLossValue(1000000, 1000, 0.0), 1);
}

TEST(Nullkiller2_Pathfinding_ArmyLoss, keepsLossFiniteForInvalidStrength)
{
	const auto loss = NK2AI::evaluateArmyLossValue(
		1000000,
		38450,
		std::numeric_limits<double>::quiet_NaN());

	EXPECT_GT(loss, 0);
	EXPECT_LT(loss, 1000000);
}

TEST(Nullkiller2_Pathfinding_ArmyLoss, rejectsDangerousZeroArmy)
{
	EXPECT_EQ(NK2AI::evaluateArmyLossValue(0, 38450, 1.0), std::numeric_limits<uint64_t>::max());
}

TEST(Nullkiller2_Pathfinding_ArmyLoss, ignoresZeroDanger)
{
	EXPECT_EQ(NK2AI::evaluateArmyLossValue(0, 0, 0.0), 0);
}
