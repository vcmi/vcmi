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

rmg::DistanceMap borderDistanceOf(const rmg::Area & area)
{
	return area.computeDistanceMap();
}

rmg::Area joinBandAround(const rmg::Area & roads, int joinDistance)
{
	rmg::Area band = roads;
	for(int r = 1; r <= joinDistance; ++r)
		band.unite(rmg::Area(band.getBorderOutside()));
	return band;
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

TEST(RmgPathTest, SearchStillPrefersCheapTilesAfterAHugePenalty)
{
	// Leaving the start costs as much as a move behind an object that is not visitable from top.
	// Later differences must not get lost in rounding against that penalty.
	const auto grid = rect(0, 0, 5, 3);
	rmg::Path path(grid);
	path.connect(int3(4, 0, 0));

	const auto penaltyThenCheapBottomRow = [](const int3 & src, const int3 & dst) -> float
	{
		if(src == int3(0, 0, 0))
			return 1e12f;
		return dst.y == 2 ? 1.f : 1000.f;
	};
	const auto result = path.search(int3(0, 0, 0), true, penaltyThenCheapBottomRow);

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

TEST(RmgPathTest, BezierCurveDistanceSqIsZeroAtEndsAndGrowsAwayFromStraightCurve)
{
	const rmg::BezierCurve curve{int3(0, 0, 0), int3(1, 0, 0), int3(2, 0, 0), int3(3, 0, 0)};

	EXPECT_FLOAT_EQ(curve.distanceSq(curve.p0), 0.f);
	EXPECT_FLOAT_EQ(curve.distanceSq(curve.p3), 0.f);

	const float oneAway = curve.distanceSq(int3(1, 1, 0));
	const float twoAway = curve.distanceSq(int3(1, 2, 0));
	const float threeAway = curve.distanceSq(int3(1, 3, 0));
	EXPECT_GT(oneAway, 0.f);
	EXPECT_GT(twoAway, oneAway);
	EXPECT_GT(threeAway, twoAway);
}

TEST(RmgPathTest, BezierCurveCostFunctionIsZeroOnCurveAndPositiveOffIt)
{
	const rmg::BezierCurve curve{int3(0, 0, 0), int3(0, 20, 0), int3(20, 20, 0), int3(20, 0, 0)};
	const auto cost = curve.costFunction();

	EXPECT_FLOAT_EQ(cost(int3(0, 0, 0), int3(10, 15, 0)), 0.f);
	EXPECT_GT(cost(int3(0, 0, 0), int3(10, 0, 0)), 0.f);
}

TEST(RmgPathTest, SearchWithBezierCostFollowsBentCurveNotTheChord)
{
	const auto grid = rect(0, 0, 21, 21);
	rmg::Path path(grid);
	path.connect(int3(20, 0, 0));

	const rmg::BezierCurve curve{int3(0, 0, 0), int3(0, 20, 0), int3(20, 20, 0), int3(20, 0, 0)};
	const auto result = path.search(int3(0, 0, 0), true, curve.costFunction());

	ASSERT_TRUE(result.valid());
	EXPECT_TRUE(result.getPathArea().contains(int3(10, 15, 0)));
	EXPECT_FALSE(result.getPathArea().contains(int3(10, 0, 0)));
}

TEST(RmgPathTest, CurveSearchJoinsRoadWithinJoinBandInsteadOfFollowingCurveToTheEnd)
{
	constexpr int JOIN_DISTANCE = 8;
	const auto grid = rect(0, 0, 25, 16);
	const int3 node(0, 2, 0);
	const int3 centre(24, 2, 0);
	const rmg::BezierCurve curve{node, int3(8, 2, 0), int3(16, 2, 0), centre};
	const auto nearbyRoad = row(8, 20, 8); // 6 tiles from the curve
	const auto farRoad = row(8, 20, 15); // 13 tiles from the curve

	rmg::Area nearbyNetwork = nearbyRoad;
	nearbyNetwork.add(centre);
	rmg::Area farNetwork = farRoad;
	farNetwork.add(centre);

	const rmg::DistanceMap noBorder;
	const auto stepCost = [](const rmg::Path::MoveCostFunction & cost)
	{
		return [cost](const int3 & src, const int3 & dst) -> float { return 1.f + cost(src, dst); };
	};
	const auto plainCost = stepCost(curve.costFunction(noBorder));

	rmg::Path nearbyPath(grid);
	nearbyPath.connect(nearbyNetwork);
	const auto plainNearby = nearbyPath.search(node, true, plainCost);
	const auto joinNearby = nearbyPath.search(node, true, stepCost(curve.costFunction(noBorder, joinBandAround(nearbyNetwork, JOIN_DISTANCE))));

	ASSERT_TRUE(plainNearby.valid());
	ASSERT_TRUE(joinNearby.valid());
	EXPECT_TRUE(plainNearby.getPathArea().contains(centre));
	EXPECT_FALSE(plainNearby.getPathArea().overlap(nearbyRoad));
	EXPECT_TRUE(joinNearby.getPathArea().overlap(nearbyRoad));
	EXPECT_FALSE(joinNearby.getPathArea().contains(centre));
	EXPECT_LE(static_cast<int>(joinNearby.getPathArea().getTiles().size()), 15);

	rmg::Path farPath(grid);
	farPath.connect(farNetwork);
	const auto plainFar = farPath.search(node, true, plainCost);
	const auto joinFar = farPath.search(node, true, stepCost(curve.costFunction(noBorder, joinBandAround(farNetwork, JOIN_DISTANCE))));

	ASSERT_TRUE(plainFar.valid());
	ASSERT_TRUE(joinFar.valid());
	EXPECT_TRUE(joinFar.getPathArea().contains(centre));
	EXPECT_FALSE(joinFar.getPathArea().overlap(farRoad));
}

TEST(RmgPathTest, BezierCurveStaysInsideARectangleInterior)
{
	const auto area = rect(0, 0, 21, 21);
	const auto borderDistance = borderDistanceOf(area);
	const rmg::BezierCurve curve{int3(5, 10, 0), int3(8, 12, 0), int3(13, 12, 0), int3(16, 10, 0)};

	EXPECT_GE(curve.minSampledRing(borderDistance, 4), 4);
}

TEST(RmgPathTest, BezierCurvePulledToTheRectangleEdgeDoesNotStayInside)
{
	const auto area = rect(0, 0, 21, 21);
	const auto borderDistance = borderDistanceOf(area);
	const rmg::BezierCurve curve{int3(5, 10, 0), int3(10, 0, 0), int3(10, 0, 0), int3(16, 10, 0)};

	EXPECT_LT(curve.minSampledRing(borderDistance, 4), 4);
}

TEST(RmgPathTest, BezierCurveWithEndpointOnTheEdgeStillStaysInside)
{
	const auto area = rect(0, 0, 21, 21);
	const auto borderDistance = borderDistanceOf(area);
	const rmg::BezierCurve curve{int3(0, 10, 0), int3(6, 12, 0), int3(10, 12, 0), int3(14, 10, 0)};

	EXPECT_GE(curve.minSampledRing(borderDistance, 4), 4);
}

TEST(RmgPathTest, SearchWithBezierBorderPenaltyStaysInlandWhileUnpenalizedPathHugsTheBorder)
{
	const auto area = rect(0, 0, 24, 10);
	const int3 start(0, 0, 0);
	const int3 end(23, 0, 0);
	const rmg::BezierCurve curve{start, int3(8, 0, 0), int3(16, 0, 0), end};

	const auto borderDistance = area.computeDistanceMap();

	rmg::Path path(area);
	path.connect(end);

	const auto hugging = path.search(start, true, curve.costFunction());
	ASSERT_TRUE(hugging.valid());
	for(const auto & tile : hugging.getPathArea().getTiles())
		EXPECT_EQ(tile.y, 0);

	const auto inland = path.search(start, true, curve.costFunction(borderDistance));
	ASSERT_TRUE(inland.valid());
	for(const auto & tile : inland.getPathArea().getTiles())
	{
		const int distStart = std::abs(tile.x - start.x) + std::abs(tile.y - start.y);
		const int distEnd = std::abs(tile.x - end.x) + std::abs(tile.y - end.y);
		if(distStart <= 3 || distEnd <= 3)
			continue;
		ASSERT_TRUE(borderDistance.count(tile));
		EXPECT_GE(borderDistance.at(tile), 2);
	}
}

}
