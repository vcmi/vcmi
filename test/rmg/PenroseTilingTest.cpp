/*
 * PenroseTilingTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/rmg/PenroseTiling.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_map>
#include <vector>

namespace test
{

using namespace ::testing;

namespace
{

struct VertexGrid
{
	float cell = 1.f;
	std::unordered_map<uint64_t, std::vector<Point2D>> bins;

	static uint64_t key(int ix, int iy)
	{
		return (static_cast<uint64_t>(static_cast<uint32_t>(ix)) << 32) | static_cast<uint32_t>(iy);
	}

	int binCoord(float value) const
	{
		return static_cast<int>(std::floor(value / cell));
	}

	explicit VertexGrid(const std::set<Point2D> & points, float cellSize)
		: cell(cellSize)
	{
		for(const auto & point : points)
			bins[key(binCoord(point.x()), binCoord(point.y()))].push_back(point);
	}

	bool hasWithin(float x, float y, float maxDist) const
	{
		const float maxDistSq = maxDist * maxDist;
		const int ix = binCoord(x);
		const int iy = binCoord(y);
		const int radius = static_cast<int>(std::ceil(maxDist / cell));
		for(int dy = -radius; dy <= radius; ++dy)
		{
			for(int dx = -radius; dx <= radius; ++dx)
			{
				const auto it = bins.find(key(ix + dx, iy + dy));
				if(it == bins.end())
					continue;
				for(const auto & point : it->second)
				{
					const float ddx = point.x() - x;
					const float ddy = point.y() - y;
					if(ddx * ddx + ddy * ddy <= maxDistSq)
						return true;
				}
			}
		}
		return false;
	}
};

double meanVertexCount(size_t numZones, float tileScale, int mapSize)
{
	PenroseTiling tiling;
	double sum = 0.0;
	for(int seed = 1; seed <= 10; ++seed)
	{
		CRandomGenerator rng(seed);
		sum += static_cast<double>(tiling.generatePenroseTiling(numZones, tileScale, mapSize, &rng).size());
	}
	return sum / 10.0;
}

void nearestNeighbourMeans(const std::set<Point2D> & points, double & meanAbsDx, double & meanAbsDy)
{
	std::vector<Point2D> vertices(points.begin(), points.end());
	double sumDx = 0.0;
	double sumDy = 0.0;
	int count = 0;
	for(size_t i = 0; i < vertices.size(); ++i)
	{
		float best = std::numeric_limits<float>::max();
		float bestDx = 0.f;
		float bestDy = 0.f;
		for(size_t j = 0; j < vertices.size(); ++j)
		{
			if(i == j)
				continue;
			const float dx = vertices[j].x() - vertices[i].x();
			const float dy = vertices[j].y() - vertices[i].y();
			const float distSq = dx * dx + dy * dy;
			if(distSq < best)
			{
				best = distSq;
				bestDx = std::abs(dx);
				bestDy = std::abs(dy);
			}
		}
		if(best < std::numeric_limits<float>::max())
		{
			sumDx += bestDx;
			sumDy += bestDy;
			++count;
		}
	}
	ASSERT_GT(count, 0);
	meanAbsDx = sumDx / count;
	meanAbsDy = sumDy / count;
}

}

TEST(PenroseTilingTest, Coverage)
{
	PenroseTiling tiling;
	const size_t zoneCounts[] = {0, 1, 2, 8, 20, 50, 131, 200};
	const float scales[] = {0.25f, 0.5f, 1.f, 2.f};
	const int mapSizes[] = {36, 252};
	const int seeds[] = {1, 2, 3};

	for(size_t numZones : zoneCounts)
	{
		for(float tileScale : scales)
		{
			for(int mapSize : mapSizes)
			{
				const auto geom = PenroseTiling::geometry(numZones, tileScale, mapSize);
				ASSERT_GT(geom.edge, 0.f) << "numZones=" << numZones << " scale=" << tileScale << " mapSize=" << mapSize;
				const float maxDist = 3.f * geom.edge;
				for(int seed : seeds)
				{
					CRandomGenerator rng(seed);
					const auto points = tiling.generatePenroseTiling(numZones, tileScale, mapSize, &rng);
					ASSERT_FALSE(points.empty()) << "numZones=" << numZones << " scale=" << tileScale << " mapSize=" << mapSize << " seed=" << seed;
					VertexGrid grid(points, std::max(geom.edge, 1.e-4f));
					for(int iy = 0; iy <= 100; ++iy)
					{
						for(int ix = 0; ix <= 100; ++ix)
						{
							const float x = ix / 100.f;
							const float y = iy / 100.f;
							EXPECT_TRUE(grid.hasWithin(x, y, maxDist))
								<< "numZones=" << numZones << " scale=" << tileScale << " mapSize=" << mapSize
								<< " seed=" << seed << " sample=(" << x << "," << y << ") edge=" << geom.edge;
							if(HasFailure())
								return;
						}
					}
				}
			}
		}
	}
}

TEST(PenroseTilingTest, BaselineTileCount)
{
	// Means of generatePenroseTiling size() over seeds 1-10, captured from the
	// pre-isotropic X-stretched tiling. Guard the area-preserving formula.
	const struct
	{
		size_t numZones;
		double mean;
	} baseline[] = {
		{1, 269.5},
		{2, 288.1},
		{4, 324.7},
		{8, 400.3},
		{12, 476.5},
		{20, 626.9},
		{50, 1191.9},
		{100, 2132.8},
	};

	for(const auto & row : baseline)
	{
		const double actual = meanVertexCount(row.numZones, 1.f, 252);
		EXPECT_NEAR(actual, row.mean, 0.10 * row.mean) << "numZones=" << row.numZones;
	}
}

TEST(PenroseTilingTest, CountScalesWithInverseSquare)
{
	const double meanAtOne = meanVertexCount(8, 1.f, 1000);
	const double meanAtHalf = meanVertexCount(8, 0.5f, 1000);
	EXPECT_NEAR(meanAtHalf, 4.0 * meanAtOne, 0.15 * 4.0 * meanAtOne)
		<< "mean(scale=1)=" << meanAtOne << " mean(scale=0.5)=" << meanAtHalf;
}

TEST(PenroseTilingTest, IsotropicNearestNeighbour)
{
	PenroseTiling tiling;
	for(size_t numZones : {size_t{2}, size_t{8}})
	{
		double sumDx = 0.0;
		double sumDy = 0.0;
		int samples = 0;
		for(int seed = 1; seed <= 3; ++seed)
		{
			CRandomGenerator rng(seed);
			const auto points = tiling.generatePenroseTiling(numZones, 1.f, 252, &rng);
			double meanDx = 0.0;
			double meanDy = 0.0;
			nearestNeighbourMeans(points, meanDx, meanDy);
			sumDx += meanDx;
			sumDy += meanDy;
			++samples;
		}
		const double meanDx = sumDx / samples;
		const double meanDy = sumDy / samples;
		const double larger = std::max(meanDx, meanDy);
		const double smaller = std::min(meanDx, meanDy);
		EXPECT_LE(larger, smaller * 1.15)
			<< "numZones=" << numZones << " mean|dx|=" << meanDx << " mean|dy|=" << meanDy;
	}
}

TEST(PenroseTilingTest, Floor)
{
	// Below scale 1 the edge does not drop under 4 map tiles...
	EXPECT_FLOAT_EQ(PenroseTiling::geometry(8, 0.25f, 252).edge, 4.0f / 252.0f);
	EXPECT_FLOAT_EQ(PenroseTiling::geometry(8, 0.25f, 1000).edge, 0.25f * PenroseTiling::geometry(8, 1.f, 1000).edge);

	// ...but the floor never makes it coarser than the scale 1 edge.
	const float L0at8 = PenroseTiling::geometry(8, 1.f, 36).edge;
	EXPECT_LT(L0at8, 4.0f / 36.0f);
	EXPECT_FLOAT_EQ(PenroseTiling::geometry(8, 0.25f, 36).edge, L0at8);
	EXPECT_FLOAT_EQ(PenroseTiling::geometry(50, 1.f, 36).edge, PenroseTiling::geometry(50, 1.f, 1000).edge);
}

TEST(PenroseTilingTest, PointsStayInsideUnitSquare)
{
	PenroseTiling tiling;
	CRandomGenerator rng(1);
	const auto points = tiling.generatePenroseTiling(8, 1.f, 252, &rng);
	ASSERT_FALSE(points.empty());
	for(const auto & point : points)
	{
		EXPECT_GE(point.x(), 0.f);
		EXPECT_LE(point.x(), 1.f);
		EXPECT_GE(point.y(), 0.f);
		EXPECT_LE(point.y(), 1.f);
	}
}

}
