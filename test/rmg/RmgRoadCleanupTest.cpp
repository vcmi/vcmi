/*
 * RmgRoadCleanupTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/Functions.h"
#include "../../lib/rmg/RmgArea.h"

namespace test
{

using namespace ::testing;

namespace
{

rmg::Area area(std::initializer_list<int3> tiles)
{
	return rmg::Area(rmg::Tileset(tiles));
}

rmg::Tileset tiles(std::initializer_list<int3> list)
{
	return rmg::Tileset(list);
}

std::vector<int3> sorted(const rmg::Area & a)
{
	std::vector<int3> result(a.getTiles().begin(), a.getTiles().end());
	std::sort(result.begin(), result.end());
	return result;
}

bool hasSquare(const rmg::Area & roads)
{
	for(const int3 & t : roads.getTiles())
	{
		if(roads.contains(t + int3(1, 0, 0)) && roads.contains(t + int3(0, 1, 0)) && roads.contains(t + int3(1, 1, 0)))
			return true;
	}
	return false;
}

bool connected4(const rmg::Area & roads)
{
	if(roads.empty())
		return true;
	rmg::Tileset unvisited = roads.getTiles();
	std::vector<int3> queue = {*unvisited.begin()};
	unvisited.erase(unvisited.begin());
	while(!queue.empty())
	{
		const int3 tile = queue.back();
		queue.pop_back();
		for(const auto & dir : rmg::dirs4)
		{
			if(unvisited.erase(tile + dir))
				queue.push_back(tile + dir);
		}
	}
	return unvisited.empty();
}

}

TEST(RmgRoadDeadEndsTest, StraightSpurOffPathIsRemoved)
{
	const int3 keepA(0, 0, 0);
	const int3 keepB(3, 0, 0);
	const auto path = area({
		keepA, int3(1, 0, 0), int3(2, 0, 0), keepB
	});
	const auto roads = path + area({
		int3(-1, 0, 0), int3(-2, 0, 0), int3(-3, 0, 0)
	});

	const auto pruned = pruneRoadDeadEnds(roads, tiles({keepA, keepB}));

	EXPECT_EQ(sorted(pruned), sorted(path));
}

TEST(RmgRoadDeadEndsTest, StaircaseSpurIsRemoved)
{
	const int3 keepA(0, 0, 0);
	const int3 keepB(2, 0, 0);
	const auto path = area({
		keepA, int3(1, 0, 0), keepB
	});
	// Orthogonal step off the keep, then a 4-connected staircase.
	const auto roads = path + area({
		int3(0, 1, 0),
		int3(0, 2, 0),
		int3(1, 2, 0),
		int3(1, 3, 0),
		int3(2, 3, 0),
		int3(2, 4, 0)
	});

	const auto pruned = pruneRoadDeadEnds(roads, tiles({keepA, keepB}));

	EXPECT_EQ(sorted(pruned), sorted(path));
}

TEST(RmgRoadDeadEndsTest, SideNubOffPathIsRemoved)
{
	const int3 keepA(0, 0, 0);
	const int3 keepB(4, 0, 0);
	const auto path = area({
		keepA, int3(1, 0, 0), int3(2, 0, 0), int3(3, 0, 0), keepB
	});
	const auto roads = path + area({int3(2, 1, 0)});

	const auto pruned = pruneRoadDeadEnds(roads, tiles({keepA, keepB}));

	EXPECT_EQ(sorted(pruned), sorted(path));
}

TEST(RmgRoadDeadEndsTest, JunctionJoiningThreeKeepBranchesStays)
{
	const int3 keepA(0, 0, 0);
	const int3 keepB(4, 0, 0);
	const int3 keepC(2, 3, 0);
	const auto roads = area({
		keepA, int3(1, 0, 0), int3(2, 0, 0), int3(3, 0, 0), keepB,
		int3(2, 1, 0), int3(2, 2, 0), keepC
	});

	const auto pruned = pruneRoadDeadEnds(roads, tiles({keepA, keepB, keepC}));

	EXPECT_EQ(sorted(pruned), sorted(roads));
}

TEST(RmgRoadDeadEndsTest, LoopWithNoTipsStays)
{
	const auto roads = area({
		int3(2, 0, 0), int3(3, 0, 0),
		int3(1, 1, 0), int3(4, 1, 0),
		int3(1, 2, 0), int3(4, 2, 0),
		int3(2, 3, 0), int3(3, 3, 0)
	});

	const auto pruned = pruneRoadDeadEnds(roads, {});

	EXPECT_EQ(sorted(pruned), sorted(roads));
}

TEST(RmgRoadSquaresTest, SquareAtJunctionLosesTheTileWithNoBranch)
{
	const int3 keepA(0, 0, 0);
	const int3 keepB(5, 0, 0);
	const int3 keepC(2, 3, 0);
	const auto junction = area({
		keepA, int3(1, 0, 0), int3(2, 0, 0), int3(3, 0, 0), int3(4, 0, 0), keepB,
		int3(2, 1, 0), int3(2, 2, 0), keepC
	});
	const auto roads = junction + area({int3(3, 1, 0)});

	const auto result = removeRoadSquares(roads, {keepA, keepB, keepC});

	EXPECT_EQ(sorted(result), sorted(junction));
}

TEST(RmgRoadSquaresTest, SquareOnTurnKeepsTheDiagonalVariant)
{
	// Road enters the square from the left and leaves downwards. Dropping either free corner keeps it
	// connected; dropping the outer corner leaves a staircase, drawn as a diagonal road.
	const int3 keepA(-2, 0, 0);
	const int3 keepB(1, 3, 0);
	const auto roads = area({
		keepA, int3(-1, 0, 0), int3(0, 0, 0), int3(1, 0, 0),
		int3(0, 1, 0), int3(1, 1, 0), int3(1, 2, 0), keepB
	});

	const auto result = removeRoadSquares(roads, {keepA, keepB});

	auto expected = roads;
	expected.erase(int3(1, 0, 0));
	EXPECT_EQ(sorted(result), sorted(expected));
}

TEST(RmgRoadSquaresTest, SquareOfKeptTilesStays)
{
	const auto roads = area({int3(0, 0, 0), int3(1, 0, 0), int3(0, 1, 0), int3(1, 1, 0)});

	const auto result = removeRoadSquares(roads, roads.getTiles());

	EXPECT_EQ(sorted(result), sorted(roads));
}

TEST(RmgRoadSquaresTest, SquareWhoseEveryTileCarriesABranchStays)
{
	// Pinwheel: each square tile is the only link of one kept branch tile.
	const rmg::Tileset keep = {int3(-1, 0, 0), int3(1, -1, 0), int3(0, 2, 0), int3(2, 1, 0)};
	const auto roads = area({int3(0, 0, 0), int3(1, 0, 0), int3(0, 1, 0), int3(1, 1, 0)}) + rmg::Area(keep);

	const auto result = removeRoadSquares(roads, keep);

	EXPECT_EQ(sorted(result), sorted(roads));
}

TEST(RmgRoadSquaresTest, WideRoadIsThinnedToConnectedRoadWithoutSquares)
{
	const int3 keepA(0, -1, 0);
	const int3 keepB(1, 4, 0);
	auto roads = area({keepA, keepB});
	for(int y = 0; y < 4; ++y)
	{
		roads.add(int3(0, y, 0));
		roads.add(int3(1, y, 0));
	}

	const auto result = removeRoadSquares(roads, {keepA, keepB});

	EXPECT_FALSE(hasSquare(result));
	EXPECT_TRUE(connected4(result));
	EXPECT_TRUE(result.contains(keepA));
	EXPECT_TRUE(result.contains(keepB));
}

}
