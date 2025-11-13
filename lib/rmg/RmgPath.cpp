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

VCMI_LIB_NAMESPACE_BEGIN

using namespace rmg;

const std::function<float(const int3 &, const int3 &)> Path::DEFAULT_MOVEMENT_FUNCTION =
[](const int3 & src, const int3 & dst)
{	
	return 1.f;
};

//A* priority queue
using TDistance = std::pair<int3, float>;
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

Path::Path(const Area & area): dArea(&area)
{
}

Path::Path(const Area & area, const int3 & src): dArea(&area)
{
	dPath.add(src);
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
	return Path({});
}

Path Path::search(const Tileset & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm
	if(!dArea)
		return Path::invalid();
	
	if(dst.empty()) // Skip construction of same area
		return Path(*dArea);

	auto resultArea = *dArea + dst;
	Path result(resultArea);

	int3 src = rmg::Area(dst).nearest(dPath);
	result.connect(src);
	
	Tileset closed;    // The set of nodes already evaluated.
	auto open = createPriorityQueue(); // The set of tentative nodes to be evaluated, initially containing the start node
	std::map<int3, int3> cameFrom;  // The map of navigated nodes.
	std::map<int3, float> distances;
	
	cameFrom[src] = int3(-1, -1, -1); //first node points to finish condition
	distances[src] = 0;
	open.push(std::make_pair(src, 0.f));
	// Cost from start along best known path.
	
	while(!open.empty())
	{
		auto node = open.top();
		open.pop();
		int3 currentNode = node.first;
		
		closed.insert(currentNode);
		
		if(dPath.contains(currentNode)) //we reached connection, stop
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
			auto computeTileScore = [&open, &closed, &cameFrom, &currentNode, &distances, &moveCostFunction, &result](const int3& pos) -> void
			{
				if(closed.count(pos))
					return;
				
				if(!result.dArea->contains(pos))
					return;
				
				float movementCost = moveCostFunction(currentNode, pos);

				float distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				int bestDistanceSoFar = std::numeric_limits<int>::max();
				auto it = distances.find(pos);
				if(it != distances.end())
					bestDistanceSoFar = static_cast<int>(it->second);
				
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
	assert(dst.dArea == dArea);
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

Path::MoveCostFunction Path::createCurvedCostFunction(const Area & border)
{
	// Capture by value to ensure the Area object persists
	return [border = border](const int3& src, const int3& dst) -> float
	{
		float ret = nonEuclideanCostFunction(src, dst, border.getCenterOfMass());
		// Route main roads far from border
		float dist = border.distanceSqr(dst);

		if(dist > 1.0f)
		{
			ret /= dist;
		}
		return ret;
	};
}

VCMI_LIB_NAMESPACE_END
