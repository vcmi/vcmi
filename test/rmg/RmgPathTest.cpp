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

#include <random>

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

rmg::Path::PassableFunction passableIn(const rmg::Area & area)
{
	return [&area](const int3 & tile)
	{
		return area.contains(tile);
	};
}

/// Area-based Path with `connected` tiles vs predicate Path over the same area with `connected` as its target.
void expectSameAsAreaPath(const rmg::Area & area, const rmg::Area & connected, const rmg::Area & dst, bool straight,
	const rmg::Path::MoveCostFunction & cost = rmg::Path::DEFAULT_MOVEMENT_FUNCTION)
{
	rmg::Path areaPath(area);
	areaPath.connect(connected);
	const rmg::Path predicatePath(passableIn(area), connected);

	const auto expected = areaPath.search(dst, straight, cost);
	const auto actual = predicatePath.search(dst, straight, cost);

	EXPECT_EQ(actual.valid(), expected.valid());
	EXPECT_EQ(sorted(actual.getPathArea()), sorted(expected.getPathArea()));
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

TEST(RmgPathTest, SearchKeepsCheaperPathInsideSameIntegerBucket)
{
	const auto grid = rect(0, 0, 3, 2);
	rmg::Path path(grid);
	path.connect(int3(2, 1, 0));

	const auto fractionalCosts = [](const int3 & src, const int3 & dst) -> float
	{
		const auto edge = [&](int x0, int y0, int x1, int y1)
		{
			return src == int3(x0, y0, 0) && dst == int3(x1, y1, 0);
		};
		if(edge(0, 0, 1, 0))
			return 0.1f;
		if(edge(0, 0, 0, 1))
			return 0.4f;
		if(edge(1, 0, 1, 1))
			return 0.8f;
		if(edge(0, 1, 1, 1))
			return 0.1f;
		if(edge(1, 1, 2, 1))
			return 0.1f;
		return 0.9f;
	};
	const auto result = path.search(int3(0, 0, 0), true, fractionalCosts);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(rmg::Area({int3(0, 0, 0), int3(0, 1, 0), int3(1, 1, 0), int3(2, 1, 0)})));
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

TEST(RmgPathTest, PredicatePathMatchesAreaPathOnCharacterizationCases)
{
	const auto cheapBottomRow = [](const int3 & src, const int3 & dst) -> float
	{
		return dst.y == 2 ? 1.f : 10.f;
	};
	auto split = row(0, 3, 0);
	split.unite(row(6, 9, 0));

	for(bool straight : {true, false})
	{
		SCOPED_TRACE(straight ? "straight" : "diagonal");
		expectSameAsAreaPath(row(0, 9, 0), rmg::Area({int3(9, 0, 0)}), rmg::Area({int3(0, 0, 0)}), straight);
		expectSameAsAreaPath(rect(0, 0, 5, 5), rmg::Area({int3(4, 4, 0)}), rmg::Area({int3(0, 0, 0)}), straight);
		expectSameAsAreaPath(split, rmg::Area({int3(9, 0, 0)}), rmg::Area({int3(0, 0, 0)}), straight);
		expectSameAsAreaPath(row(0, 9, 0), rmg::Area({int3(0, 0, 0), int3(9, 0, 0)}), rmg::Area({int3(6, 0, 0)}), straight);
		expectSameAsAreaPath(row(0, 9, 0), rmg::Area({int3(9, 0, 0)}), rmg::Area({int3(0, 0, 0), int3(5, 0, 0)}), straight);
		expectSameAsAreaPath(row(1, 9, 0), rmg::Area({int3(9, 0, 0)}), rmg::Area({int3(0, 0, 0)}), straight);
		expectSameAsAreaPath(rect(0, 0, 5, 3), rmg::Area({int3(4, 0, 0)}), rmg::Area({int3(0, 0, 0)}), straight, cheapBottomRow);
		expectSameAsAreaPath(rect(0, 0, 10, 10), rect(9, 0, 1, 10), rmg::Area({int3(0, 5, 0)}), straight);
		expectSameAsAreaPath(row(0, 9, 0), row(5, 9, 0), rmg::Area({int3(7, 0, 0)}), straight);
	}
}

TEST(RmgPathTest, PredicatePathMatchesAreaPathOnRandomGrids)
{
	constexpr int SIZE = 24;
	std::mt19937 rng(169);
	std::uniform_int_distribution<int> coordinate(0, SIZE - 1);
	std::uniform_int_distribution<int> percent(0, 99);
	std::uniform_int_distribution<int> count(1, 4);

	for(int iteration = 0; iteration < 200; ++iteration)
	{
		SCOPED_TRACE(iteration);
		const int blockedPercent = 10 + iteration % 40;
		rmg::Area area;
		for(int y = 0; y < SIZE; ++y)
		{
			for(int x = 0; x < SIZE; ++x)
			{
				if(percent(rng) >= blockedPercent)
					area.add(int3(x, y, 0));
			}
		}

		rmg::Area connected;
		for(int i = count(rng); i > 0; --i)
			connected.add(int3(coordinate(rng), coordinate(rng), 0));
		rmg::Area dst;
		for(int i = count(rng); i > 0; --i)
			dst.add(int3(coordinate(rng), coordinate(rng), 0));

		const auto cost = [](const int3 & src, const int3 & to) -> float
		{
			return static_cast<float>(1 + (to.x * 7 + to.y * 3) % 4);
		};
		expectSameAsAreaPath(area, connected, dst, iteration % 2 == 0);
		expectSameAsAreaPath(area, connected, dst, iteration % 2 == 1, cost);
	}
}

TEST(RmgPathTest, PredicatePathEndsAtTargetOrConnectedTile)
{
	const auto corridor = row(0, 9, 0);
	const rmg::Area target({int3(0, 0, 0)});
	rmg::Path path(passableIn(corridor), target);
	path.connect(int3(9, 0, 0));

	const auto towardConnected = path.search(int3(6, 0, 0), true);
	ASSERT_TRUE(towardConnected.valid());
	EXPECT_EQ(sorted(towardConnected.getPathArea()), sorted(row(6, 9, 0)));

	const auto towardTarget = path.search(int3(2, 0, 0), true);
	ASSERT_TRUE(towardTarget.valid());
	EXPECT_EQ(sorted(towardTarget.getPathArea()), sorted(row(0, 2, 0)));
}

TEST(RmgPathTest, PredicatePathReadsTargetByReference)
{
	const auto corridor = row(0, 9, 0);
	rmg::Area target({int3(9, 0, 0)});
	const rmg::Path path(passableIn(corridor), target);

	target.clear();
	target.add(int3(3, 0, 0));
	const auto result = path.search(int3(0, 0, 0), true);

	ASSERT_TRUE(result.valid());
	EXPECT_EQ(sorted(result.getPathArea()), sorted(row(0, 3, 0)));
}

TEST(RmgPathTest, PredicatePathResultCanSearchAgain)
{
	const auto grid = rect(0, 0, 10, 3);
	const rmg::Area target({int3(9, 0, 0)});
	rmg::Path path(passableIn(grid), target);

	auto first = path.search(int3(0, 0, 0), true);
	ASSERT_TRUE(first.valid());

	const auto second = first.search(int3(5, 2, 0), true);
	ASSERT_TRUE(second.valid());
	EXPECT_EQ(sorted(second.getPathArea()), sorted(rmg::Area({int3(5, 2, 0), int3(5, 1, 0), int3(5, 0, 0)})));
}

TEST(RmgPathTest, PredicatePathRejectsTilesThePredicateRejects)
{
	const auto grid = rect(0, 0, 5, 3);
	const auto noMiddleRow = [&grid](const int3 & tile)
	{
		return grid.contains(tile) && tile.y != 1;
	};
	const rmg::Area target({int3(4, 2, 0)});
	const rmg::Path path(noMiddleRow, target);

	EXPECT_FALSE(path.search(int3(0, 0, 0), true).valid());
	EXPECT_TRUE(path.search(int3(0, 2, 0), true).valid());
}

}
