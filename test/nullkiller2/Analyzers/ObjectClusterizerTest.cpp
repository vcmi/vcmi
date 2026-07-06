/*
 * ObjectClusterizerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Analyzers/ObjectClusterizer.h"

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorReachScalesRangeByAvailableCasts)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 3, 2, 2}
	};

	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(16, 14, 0)));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(17, 14, 0)));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(16, 15, 0)));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorExpansionDoesNotReachOutsideCastEnvelope)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 4, 4, 1}
	};

	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(14, 14, 0)));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(15, 14, 0)));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(14, 15, 0)));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorExpansionUsesAnyReachableHero)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 1, 2, 2},
		{int3(20, 20, 0), 2, 3, 3}
	};

	EXPECT_TRUE(NK2AI::canReachWithDimensionDoor(reach, int3(26, 26, 0)));
	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(27, 26, 0)));
}

TEST(Nullkiller2_Analyzers_ObjectClusterizer, dimensionDoorReachDoesNotCrossLevels)
{
	std::vector<NK2AI::DimensionDoorExpansionReach> reach = {
		{int3(10, 10, 0), 3, 2, 2}
	};

	EXPECT_FALSE(NK2AI::canReachWithDimensionDoor(reach, int3(10, 10, 1)));
}
