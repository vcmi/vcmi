/*
 * RmgPathTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/RmgPath.h"

namespace test
{

using namespace ::testing;

namespace
{

rmg::Area rect(int x0, int y0, int width, int height)
{
	rmg::Tileset tiles;
	for(int y = y0; y < y0 + height; ++y)
	{
		for(int x = x0; x < x0 + width; ++x)
			tiles.insert(int3(x, y, 0));
	}
	return rmg::Area(tiles);
}

rmg::Area row(int x0, int x1, int y)
{
	return rect(x0, y, x1 - x0 + 1, 1);
}

std::vector<int3> sorted(const rmg::Area & area)
{
	std::vector<int3> tiles(area.getTiles().begin(), area.getTiles().end());
	std::sort(tiles.begin(), tiles.end());
	return tiles;
}

}

TEST(RmgPathTest, StraightCorridorPathCoversCorridor)
{
	const auto corridor = row(0, 9, 0);
	rmg::Path path(corridor);
	path.connect(int3(9, 0, 0));

	const auto result = path.search(int3(0, 0, 0), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(corridor));
}

TEST(RmgPathTest, DiagonalShortcutOnlyWhenNotStraight)
{
	const auto square = rect(0, 0, 5, 5);
	rmg::Path path(square);
	path.connect(int3(4, 4, 0));

	const auto diagonal = path.search(int3(0, 0, 0), false);
	ASSERT_TRUE(diagonal.valid());
	rmg::Area expectedDiagonal;
	for(int i = 0; i < 5; ++i)
		expectedDiagonal.add(int3(i, i, 0));
	EXPECT_EQ(sorted(diagonal.getPathArea()), sorted(expectedDiagonal));

	const auto straight = path.search(int3(0, 0, 0), true);
	ASSERT_TRUE(straight.valid());
	EXPECT_EQ(straight.getPathArea().getTiles().size(), 9);
	EXPECT_TRUE(straight.getPathArea().connected(true));
	EXPECT_TRUE(straight.getPathArea().contains(int3(0, 0, 0)));
	EXPECT_TRUE(straight.getPathArea().contains(int3(4, 4, 0)));
}

TEST(RmgPathTest, UnreachableTargetGivesInvalidPath)
{
	auto area = row(0, 3, 0);
	area.unite(row(6, 9, 0));
	rmg::Path path(area);
	path.connect(int3(9, 0, 0));

	const auto result = path.search(int3(0, 0, 0), true);

	EXPECT_FALSE(result.valid());
	EXPECT_TRUE(result.getPathArea().empty());
}

TEST(RmgPathTest, EndsAtNearestOfSeveralConnectedTiles)
{
	const auto corridor = row(0, 9, 0);
	rmg::Path path(corridor);
	path.connect(int3(0, 0, 0));
	path.connect(int3(9, 0, 0));

	const auto result = path.search(int3(6, 0, 0), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(row(6, 9, 0)));
}

TEST(RmgPathTest, StartsFromSourceTileNearestToConnectedTiles)
{
	const auto corridor = row(0, 9, 0);
	rmg::Path path(corridor);
	path.connect(int3(9, 0, 0));

	const auto result = path.search(rmg::Area({int3(0, 0, 0), int3(5, 0, 0)}), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(row(5, 9, 0)));
}

TEST(RmgPathTest, SourceOutsideAreaIsStillUsedAsStart)
{
	const auto corridor = row(1, 9, 0);
	rmg::Path path(corridor);
	path.connect(int3(9, 0, 0));

	const auto result = path.search(int3(0, 0, 0), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(row(0, 9, 0)));
}

TEST(RmgPathTest, CostFunctionMakesSearchPreferCheapTiles)
{
	const auto grid = rect(0, 0, 5, 3);
	rmg::Path path(grid);
	path.connect(int3(4, 0, 0));

	const auto cheapBottomRow = [](const int3 & src, const int3 & dst) -> float
	{
		return dst.y == 2 ? 1.f : 10.f;
	};
	const auto result = path.search(int3(0, 0, 0), true, cheapBottomRow);

	ASSERT_TRUE(result.valid());
	rmg::Area expected = row(0, 4, 2);
	expected.unite(rmg::Area({int3(0, 0, 0), int3(0, 1, 0), int3(4, 1, 0), int3(4, 0, 0)}));
	EXPECT_EQ(sorted(result.getPathArea()), sorted(expected));
}

TEST(RmgPathTest, SearchFromPathWithConnectedAreaReturnsOnlyNewPath)
{
	const auto grid = rect(0, 0, 10, 10);
	rmg::Path path(grid);
	path.connect(rect(9, 0, 1, 10));

	const auto result = path.search(int3(0, 5, 0), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(row(0, 9, 5)));
	EXPECT_EQ(sorted(path.getPathArea()), sorted(rect(9, 0, 1, 10)));
}

TEST(RmgPathTest, SourceOnConnectedTileGivesSingleTilePath)
{
	const auto corridor = row(0, 9, 0);
	rmg::Path path(corridor);
	path.connect(row(5, 9, 0));

	const auto result = path.search(int3(7, 0, 0), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(rmg::Area({int3(7, 0, 0)})));
}

}
