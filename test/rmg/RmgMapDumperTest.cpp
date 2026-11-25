/*
 * RmgMapDumperTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/RmgMapDumper.h"

namespace test
{

using namespace ::testing;

TEST(RmgDumpStagesTest, FiresOnceWhenLastExpectedInstanceFinishes)
{
	RmgDumpStages stages({"TreasurePlacer"});
	stages.expect("TreasurePlacer");
	stages.expect("TreasurePlacer");
	stages.expect("TreasurePlacer");

	EXPECT_FALSE(stages.finish("TreasurePlacer"));
	EXPECT_FALSE(stages.finish("TreasurePlacer"));
	EXPECT_TRUE(stages.finish("TreasurePlacer"));
	EXPECT_FALSE(stages.finish("TreasurePlacer"));
}

TEST(RmgDumpStagesTest, UnrequestedStageNeverFires)
{
	RmgDumpStages stages({"TreasurePlacer"});
	stages.expect("ObstaclePlacer");
	stages.expect("ObstaclePlacer");

	EXPECT_FALSE(stages.finish("ObstaclePlacer"));
	EXPECT_FALSE(stages.finish("ObstaclePlacer"));
}

TEST(RmgDumpStagesTest, UnknownListsUnexpectedRequestedNamesAndOmitsZones)
{
	RmgDumpStages stages({"zones", "NoSuchStage", "TreasurePlacer"});
	stages.expect("TreasurePlacer");

	EXPECT_EQ(stages.unknown(), std::vector<std::string>{"NoSuchStage"});
}

TEST(RenderDumpGridTest, PlacesXOnColumnsYOnRowsAndBlankLineBetweenLevels)
{
	const auto encode = [](const int3 & tile) -> char
	{
		return static_cast<char>('A' + tile.x + tile.y * 3 + tile.z * 6);
	};

	EXPECT_EQ(renderDumpGrid(3, 2, 2, encode), "ABC\nDEF\n\nGHI\nJKL\n");
}

}
