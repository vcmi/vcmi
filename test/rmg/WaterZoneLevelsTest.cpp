/*
 * WaterZoneLevelsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/Functions.h"

namespace test
{

using namespace ::testing;

class WaterZoneLevelsTest : public Test
{
protected:
	const std::vector<MapLayerId> defaultLayers{MapLayerId::SURFACE, MapLayerId::UNDERGROUND, MapLayerId::UNKNOWN};
};

TEST_F(WaterZoneLevelsTest, IslandsLandOnAllLevels)
{
	const std::vector expected{0, 2};
	EXPECT_EQ(waterZoneLevels(EWaterContent::ISLANDS, defaultLayers, {0, 1, 2}), expected);
}

TEST_F(WaterZoneLevelsTest, NormalLandOnSurfaceAndUnderground)
{
	const std::vector expected{0};
	EXPECT_EQ(waterZoneLevels(EWaterContent::NORMAL, defaultLayers, {0, 1}), expected);
}

TEST_F(WaterZoneLevelsTest, NoneYieldsEmpty)
{
	const std::vector<int> expected{};
	EXPECT_EQ(waterZoneLevels(EWaterContent::NONE, defaultLayers, {0, 1, 2}), expected);
}

TEST_F(WaterZoneLevelsTest, IslandsSkipsLevelWithoutLand)
{
	const std::vector expected{0};
	EXPECT_EQ(waterZoneLevels(EWaterContent::ISLANDS, defaultLayers, {0, 1}), expected);
}

TEST_F(WaterZoneLevelsTest, SkipsUndergroundWhenItIsLevelZero)
{
	const std::vector<MapLayerId> layers{MapLayerId::UNDERGROUND, MapLayerId::SURFACE};
	const std::vector expected{1};
	EXPECT_EQ(waterZoneLevels(EWaterContent::ISLANDS, layers, {0, 1}), expected);
}

}
