/*
 * RmgRoadDeadEndsTest.cpp, part of VCMI engine
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

}
