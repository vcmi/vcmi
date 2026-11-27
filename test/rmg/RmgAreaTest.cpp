/*
 * RmgAreaTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/RmgArea.h"

namespace test
{
using namespace ::testing;

TEST(RmgAreaTest, EmptyAreaIsConnected)
{
	/*
	 * Asserts: a default-constructed Area and an Area emptied by subtract are
	 * trivially connected, including with noDiagonals.
	 * Pass: both connected() overloads return true.
	 * Fail: a crash or false, which used to happen on *dTiles.begin() of an empty set.
	 */

	rmg::Area empty;
	EXPECT_TRUE(empty.connected());
	EXPECT_TRUE(empty.connected(true));

	rmg::Area area;
	area.add(int3(3, 4, 0));
	rmg::Area blocker;
	blocker.add(int3(3, 4, 0));
	area.subtract(blocker);
	EXPECT_TRUE(area.empty());
	EXPECT_TRUE(area.connected());
	EXPECT_TRUE(area.connected(true));
}

TEST(RmgAreaTest, SingleTileIsConnected)
{
	rmg::Area area;
	area.add(int3(0, 0, 0));
	EXPECT_TRUE(area.connected());
	EXPECT_TRUE(area.connected(true));
}

TEST(RmgAreaTest, DiagonalTilesRespectNoDiagonals)
{
	rmg::Area area;
	area.add(int3(0, 0, 0));
	area.add(int3(1, 1, 0));
	EXPECT_TRUE(area.connected());
	EXPECT_FALSE(area.connected(true));
}

TEST(RmgAreaTest, DistantTilesAreNotConnected)
{
	rmg::Area area;
	area.add(int3(0, 0, 0));
	area.add(int3(5, 0, 0));
	EXPECT_FALSE(area.connected());
	EXPECT_FALSE(area.connected(true));
}

}
