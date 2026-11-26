/*
 * RmgPath.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RmgPath.h"
#include <boost/heap/priority_queue.hpp> //A*

using namespace rmg;

const std::function<float(const int3 &, const int3 &)> Path::DEFAULT_MOVEMENT_FUNCTION =
[](const int3 & src, const int3 & dst)
{	
	return 1.f;
};

//A* priority queue
using TDistance = std::pair<int3, double>;
struct NodeComparer
{
	bool operator()(const TDistance & lhs, const TDistance & rhs) const
	{
		return (rhs.second < lhs.second);
	}
};
boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue()
{
	return boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>>();
}

Path::Path(const Area & area)
	: dPassable([&area](const int3 & tile){ return area.contains(tile); })
{
}

Path::Path(const Area & area, const int3 & src)
	: Path(area)
{
	dPath.add(src);
}

Path::Path(PassableFunction passable, const Area & target)
	: dPassable(std::move(passable))
	, dTarget(&target)
{
}

Path & Path::operator= (const Path & path)
{
	//do not modify area
	dPath = path.dPath;
	return *this;
}

bool Path::valid() const
{
	return !dPath.empty();
}

Path Path::invalid()
{
	return Path();
}

bool Path::isTarget(const int3 & tile) const
{
	return dPath.contains(tile) || (dTarget && dTarget->contains(tile));
}

int3 Path::nearestTarget(const int3 & tile) const
{
	int3 result = dPath.empty() ? int3(-1, -1, -1) : dPath.nearest(tile);
	if(dTarget && !dTarget->empty())
	{
		int3 targetTile = dTarget->nearest(tile);
		if(!result.isValid() || targetTile.dist2dSQ(tile) < result.dist2dSQ(tile))
			result = targetTile;
	}
	return result;
}

Path Path::search(const Tileset & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm
	if(!dPassable)
		return Path::invalid();
	
	Path result(*this);
	result.dPath.clear();
	if(dst.empty())
		return result;

	// Same alternation as Area::nearest, over dPath and dTarget together
	const rmg::Area dstArea(dst);
	int dist = std::numeric_limits<int>::max();
	int3 src = dstArea.getTilesVector().front();
	int3 otherNearTile = nearestTarget(src);
	while(dist != otherNearTile.dist2dSQ(src))
	{
		dist = otherNearTile.dist2dSQ(src);
		src = dstArea.nearest(otherNearTile);
		otherNearTile = nearestTarget(src);
	}
	result.connect(src);
	
	Tileset closed;    // The set of nodes already evaluated.
	auto open = createPriorityQueue(); // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, double> distances;
	
	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	open.push(std::make_pair(src, 0.0));
	// Cost from start along best known path.
	
	while(!open.empty())
	{
		auto node = open.top();
		open.pop();
		int3 currentNode = node.first;
		
		closed.insert(currentNode);
		
		if(isTarget(currentNode)) //we reached connection, stop
		{
			// Trace the path using the saved parent information and return path
			int3 backTracking = currentNode;
			while (cameFrom[backTracking].isValid())
			{
				result.dPath.add(backTracking);
				backTracking = cameFrom[backTracking];
			}
			return result;
		}
		else
		{
			auto computeTileScore = [this, &open, &closed, &cameFrom, &currentNode, &distances, &moveCostFunction, &dst](const int3& pos) -> void
			{
				if(closed.count(pos))
					return;
				
				if(!dst.count(pos) && !dPassable(pos))
					return;
				
				float movementCost = moveCostFunction(currentNode, pos);

				double distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				double bestDistanceSoFar = std::numeric_limits<double>::max();
				auto it = distances.find(pos);
				if(it != distances.end())
					bestDistanceSoFar = it->second;
				
				if(distance < bestDistanceSoFar)
				{
					cameFrom[pos] = currentNode;
					open.push(std::make_pair(pos, distance));
					distances[pos] = distance;
				}
			};
			
			auto dirs = int3::getDirs();
			std::vector<int3> neighbors(dirs.begin(), dirs.end());
			if(straight)
				neighbors.assign(rmg::dirs4.begin(), rmg::dirs4.end());
			for(auto & i : neighbors)
			{
				computeTileScore(currentNode + i);
			}
		}
		
	}
	
	result.dPath.clear();
	return result;
}

Path Path::search(const int3 & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	return search(Tileset{dst}, straight, std::move(moveCostFunction));
}

Path Path::search(const Area & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	return search(dst.getTiles(), straight, std::move(moveCostFunction));
}

Path Path::search(const Path & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	return search(dst.dPath, straight, std::move(moveCostFunction));
}

void Path::connect(const int3 & path)
{
	dPath.add(path);
}

void Path::connect(const Tileset & path)
{
	Area a(path);
	dPath.unite(a);
}

void Path::connect(const Area & path)
{
	dPath.unite(path);
}

void Path::connect(const Path & path)
{
	dPath.unite(path.dPath);
}

const Area & Path::getPathArea() const
{
	return dPath;
}

float Path::nonEuclideanCostFunction(const int3& src, const int3& dst, const int3& center)
{
	float R = 30.0f; // radius of the zone
	float W = 10.0f;// width of the transition area
	float A = 0.7f; // sine bias

	// Euclidean distance:
	float d = src.dist2d(dst);

	// Distance from dst to the zone center:
	float r = dst.dist2d(center);

	// Compute normalized offset inside the zone:
	// (R - W) is the inner edge, R is the outer edge.
	float t = std::clamp((r - (R - W)) / W, 0.0f, 1.0f);

	// Use sine bias: lowest cost in the middle (t=0.5), higher near edges.
	float bias = 1.0f + A * std::sin(M_PI * t);

	return d * bias;
}

namespace
{
constexpr int BEZIER_SAMPLES = 20;

std::pair<float, float> evaluate(const BezierCurve & curve, float t)
{
	const float oneMinusT = 1.0f - t;
	const float oneMinusT2 = oneMinusT * oneMinusT;
	const float oneMinusT3 = oneMinusT2 * oneMinusT;
	const float t2 = t * t;
	const float t3 = t2 * t;

	const float bx = oneMinusT3 * curve.p0.x + 3.0f * oneMinusT2 * t * curve.p1.x + 3.0f * oneMinusT * t2 * curve.p2.x + t3 * curve.p3.x;
	const float by = oneMinusT3 * curve.p0.y + 3.0f * oneMinusT2 * t * curve.p1.y + 3.0f * oneMinusT * t2 * curve.p2.y + t3 * curve.p3.y;
	return {bx, by};
}
}

int3 BezierCurve::pointAt(float t) const
{
	const auto [bx, by] = evaluate(*this, t);
	return int3(static_cast<si32>(std::lround(bx)), static_cast<si32>(std::lround(by)), p0.z);
}

float BezierCurve::distanceSq(const int3 & tile) const
{
	float minDistSq = std::numeric_limits<float>::max();

	for(int i = 0; i <= BEZIER_SAMPLES; ++i)
	{
		const auto [bx, by] = evaluate(*this, static_cast<float>(i) / BEZIER_SAMPLES);
		const float dx = tile.x - bx;
		const float dy = tile.y - by;
		const float distSq = dx * dx + dy * dy;

		if(distSq < minDistSq)
			minDistSq = distSq;
	}

	return minDistSq;
}

int BezierCurve::minSampledRing(const DistanceMap & borderDistance, int minRing) const
{
	const ui32 skipDistSq = static_cast<ui32>(minRing + 1) * static_cast<ui32>(minRing + 1);
	int worst = std::numeric_limits<int>::max();

	for(int i = 0; i <= BEZIER_SAMPLES; ++i)
	{
		const int3 sample = pointAt(static_cast<float>(i) / BEZIER_SAMPLES);
		if(sample.dist2dSQ(p0) <= skipDistSq || sample.dist2dSQ(p3) <= skipDistSq)
			continue;

		const auto it = borderDistance.find(sample);
		const int ring = (it == borderDistance.end()) ? -1 : it->second;
		worst = std::min(worst, ring);
	}

	return worst;
}

bool BezierCurve::staysInside(const DistanceMap & borderDistance, int minRing) const
{
	return minSampledRing(borderDistance, minRing) >= minRing;
}

float BezierCurve::borderPenalty(const DistanceMap & borderDistance, const int3 & tile)
{
	constexpr int R = 8; // tiles from the zone border that still receive a penalty
	constexpr float K = 16.f; // strength of the additive per-step border penalty
	const auto it = borderDistance.find(tile);
	if(it == borderDistance.end())
		return 0.f;
	const int inward = std::max(0, R - it->second);
	return K * static_cast<float>(inward * inward);
}

Path::MoveCostFunction BezierCurve::costFunction() const
{
	return costFunction(DistanceMap{});
}

Path::MoveCostFunction BezierCurve::costFunction(const DistanceMap & borderDistance) const
{
	return [*this, borderDistance](const int3 &, const int3 & dst) -> float
	{
		float distToCurve = distanceSq(dst);
		return distToCurve * distToCurve + borderPenalty(borderDistance, dst);
	};
}

Path::MoveCostFunction BezierCurve::costFunction(const DistanceMap & borderDistance, const Area & joinBand) const
{
	return [*this, borderDistance, joinBand](const int3 &, const int3 & dst) -> float
	{
		float deviation = 0.f;
		if(!joinBand.contains(dst))
		{
			const float distToCurve = distanceSq(dst);
			deviation = distToCurve * distToCurve;
		}
		return deviation + borderPenalty(borderDistance, dst);
	};
}
