/*
 * AIPathNodePoolTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Pathfinding/AINodeStorage.h"

namespace
{

using NK2AI::AIPathNodePool;

TEST(Nullkiller2_Pathfinding_AIPathNodePool, allocatesOnlyOccupiedTiles)
{
	AIPathNodePool pool(int3(4, 4, 1), 2);
	EXPECT_FALSE(pool.beginGeneration());

	const int3 occupied(2, 1, 0);
	auto * first = pool.allocate(occupied);

	ASSERT_NE(first, nullptr);
	EXPECT_EQ(first->coord, occupied);
	EXPECT_EQ(pool.get(occupied), std::vector<NK2AI::AIPathNode *>({ first }));
	EXPECT_TRUE(pool.get(int3(0, 0, 0)).empty());
}

TEST(Nullkiller2_Pathfinding_AIPathNodePool, preservesLogicalSlotOrder)
{
	AIPathNodePool pool(int3(4, 4, 1), 2);
	pool.beginGeneration();

	const int3 tile(2, 1, 0);
	const auto firstOrder = pool.nextStorageOrder(tile);
	auto * first = pool.allocate(tile);
	const auto secondOrder = pool.nextStorageOrder(tile);
	auto * second = pool.allocate(tile);

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_EQ(first->storageOrder, firstOrder);
	EXPECT_EQ(second->storageOrder, secondOrder);
	EXPECT_EQ(first->storageOrder + 1, second->storageOrder);
	EXPECT_EQ(pool.get(tile), std::vector<NK2AI::AIPathNode *>({ first, second }));
	EXPECT_EQ(pool.allocate(tile), nullptr);
}

TEST(Nullkiller2_Pathfinding_AIPathNodePool, ordersPendingSlotsAfterAllocatedNodes)
{
	AIPathNodePool pool(int3(4, 4, 1), 4);
	pool.beginGeneration();

	const int3 tile(2, 1, 0);
	auto * allocated = pool.allocate(tile);

	ASSERT_NE(allocated, nullptr);
	EXPECT_EQ(pool.nextStorageOrder(tile), allocated->storageOrder + 1);
	EXPECT_EQ(pool.nextStorageOrder(tile, 1), allocated->storageOrder + 2);
}

TEST(Nullkiller2_Pathfinding_AIPathNodePool, keepsPointersStable)
{
	AIPathNodePool pool(int3(64, 1, 1), 1);
	pool.beginGeneration();

	auto * first = pool.allocate(int3(0, 0, 0));
	ASSERT_NE(first, nullptr);

	for(int x = 1; x < 64; ++x)
		ASSERT_NE(pool.allocate(int3(x, 0, 0)), nullptr);

	EXPECT_EQ(first->coord, int3(0, 0, 0));
	EXPECT_EQ(pool.get(int3(0, 0, 0)).front(), first);
}

TEST(Nullkiller2_Pathfinding_AIPathNodePool, resetsTilesByGeneration)
{
	AIPathNodePool pool(int3(2, 2, 1), 2);
	pool.beginGeneration();
	ASSERT_NE(pool.allocate(int3(1, 1, 0)), nullptr);

	EXPECT_FALSE(pool.beginGeneration());
	EXPECT_TRUE(pool.get(int3(1, 1, 0)).empty());

	auto * current = pool.allocate(int3(1, 1, 0));
	ASSERT_NE(current, nullptr);
	EXPECT_EQ(pool.get(int3(1, 1, 0)).front(), current);
}

}
