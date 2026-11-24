/*
 * TileDistanceIndexTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/rmg/TileDistanceIndex.h"

#include <algorithm>
#include <limits>
#include <random>
#include <unordered_map>
#include <unordered_set>

namespace test
{

using namespace ::testing;

constexpr int GRID = 30;
const float INF = static_cast<float>(std::numeric_limits<int>::max());

std::vector<std::pair<int3, float>> fullGrid(float distance)
{
	std::vector<std::pair<int3, float>> tiles;
	tiles.reserve(GRID * GRID);
	for(int y = 0; y < GRID; ++y)
	{
		for(int x = 0; x < GRID; ++x)
			tiles.emplace_back(int3(x, y, 0), distance);
	}
	return tiles;
}

std::vector<std::pair<int3, float>> visitAll(const rmg::TileDistanceIndex & index)
{
	std::vector<std::pair<int3, float>> visited;
	index.forEachFarthestFirst([&visited](const int3 & tile, float distance)
	{
		visited.emplace_back(tile, distance);
		return true;
	});
	return visited;
}

float bruteMin(const int3 & tile, const std::vector<int3> & sources)
{
	float best = INF;
	for(const auto & source : sources)
		best = std::min(best, static_cast<float>(tile.dist2dSQ(source)));
	return best;
}

TEST(TileDistanceIndexTest, VisitsFarthestFirstAndStops)
{
	rmg::TileDistanceIndex index;
	std::vector<std::pair<int3, float>> tiles = {
		{int3(0, 0, 0), 1.f},
		{int3(1, 0, 0), 9.f},
		{int3(2, 0, 0), 4.f},
		{int3(3, 0, 0), 16.f},
		{int3(4, 0, 0), 4.f},
	};
	index.reset(tiles);

	const auto visited = visitAll(index);
	ASSERT_EQ(visited.size(), tiles.size());
	for(size_t i = 1; i < visited.size(); ++i)
		EXPECT_GE(visited[i - 1].second, visited[i].second);

	std::vector<int3> prefix;
	index.forEachFarthestFirst([&prefix](const int3 & tile, float)
	{
		prefix.push_back(tile);
		return prefix.size() < 3;
	});
	ASSERT_EQ(prefix.size(), 3u);
	EXPECT_EQ(prefix[0], visited[0].first);
	EXPECT_EQ(prefix[1], visited[1].first);
	EXPECT_EQ(prefix[2], visited[2].first);
}

TEST(TileDistanceIndexTest, TieOrderDependsOnlyOnCoordinates)
{
	rmg::TileDistanceIndex index;
	auto tiles = fullGrid(4.f);
	auto shuffled = tiles;
	std::mt19937 rng(20261123);
	std::shuffle(shuffled.begin(), shuffled.end(), rng);

	index.reset(tiles);
	const auto first = visitAll(index);
	ASSERT_FALSE(first.empty());

	index.reset(shuffled);
	const auto second = visitAll(index);
	ASSERT_EQ(first.size(), second.size());
	for(size_t i = 0; i < first.size(); ++i)
	{
		EXPECT_EQ(first[i].first, second[i].first);
		EXPECT_EQ(first[i].second, second[i].second);
	}
}

TEST(TileDistanceIndexTest, LowerTowardsMatchesBruteForceAndReturnsLoweredTiles)
{
	rmg::TileDistanceIndex index;
	index.reset(fullGrid(INF));
	std::mt19937 rng(169);
	std::uniform_int_distribution coord(0, GRID - 1);
	std::uniform_int_distribution sourceSize(1, 5);

	std::vector<int3> allSources;
	std::unordered_map<int3, float> current;
	for(const auto & tile : fullGrid(INF))
		current[tile.first] = tile.second;

	const auto keep = [](const int3 &) { return true; };

	for(int round = 0; round < 8; ++round)
	{
		std::vector<int3> source;
		const int n = sourceSize(rng);
		source.reserve(n);
		for(int i = 0; i < n; ++i)
			source.emplace_back(coord(rng), coord(rng), 0);
		allSources.insert(allSources.end(), source.begin(), source.end());

		std::unordered_set<int3> expectedLowered;
		for(auto & [tile, distance] : current)
		{
			const float next = bruteMin(tile, source);
			if(next < distance)
			{
				distance = next;
				expectedLowered.insert(tile);
			}
		}

		const auto lowered = index.lowerTowards(source, keep);
		std::unordered_set<int3> returned;
		for(const auto & [tile, distance] : lowered)
		{
			EXPECT_TRUE(returned.insert(tile).second);
			ASSERT_TRUE(current.contains(tile));
			EXPECT_FLOAT_EQ(distance, current[tile]);
		}
		EXPECT_EQ(returned, expectedLowered);

		const auto visited = visitAll(index);
		ASSERT_EQ(visited.size(), current.size());
		for(size_t i = 1; i < visited.size(); ++i)
			EXPECT_GE(visited[i - 1].second, visited[i].second);

		std::unordered_set<int3> seen;
		for(const auto & [tile, distance] : visited)
		{
			EXPECT_TRUE(seen.insert(tile).second);
			EXPECT_FLOAT_EQ(distance, bruteMin(tile, allSources));
		}
		EXPECT_EQ(seen.size(), current.size());
	}
}

TEST(TileDistanceIndexTest, DropsTilesThatFailKeepInsideScannedBox)
{
	rmg::TileDistanceIndex index;
	index.reset(fullGrid(INF));

	std::unordered_set dropped = {
		int3(10, 10, 0), int3(11, 10, 0), int3(12, 11, 0),
		int3(0, 0, 0), int3(29, 29, 0), int3(5, 20, 0),
	};
	const std::vector source = {int3(10, 10, 0), int3(11, 11, 0)};
	const auto keep = [&dropped](const int3 & tile) { return !dropped.contains(tile); };

	const auto lowered = index.lowerTowards(source, keep);
	for(const auto & [tile, distance] : lowered)
	{
		EXPECT_FALSE(dropped.contains(tile));
		EXPECT_LT(distance, INF);
	}

	const auto visited = visitAll(index);
	ASSERT_FALSE(visited.empty());
	std::unordered_set<int3> seen;
	for(const auto & [tile, distance] : visited)
	{
		EXPECT_FALSE(dropped.contains(tile));
		EXPECT_TRUE(seen.insert(tile).second);
	}
	EXPECT_EQ(seen.size(), static_cast<size_t>(GRID * GRID) - dropped.size());
}

}
