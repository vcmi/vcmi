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

Path::Path(const Path & path) = default;

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
	
	auto resultArea = *dArea + dst;
	Path result(resultArea);
	if(dst.empty())
		return result;
	
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
			while (cameFrom[backTracking].valid())
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
				
				float movementCost = moveCostFunction(currentNode, pos) + currentNode.dist2d(pos);
				
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
	return search(Tileset{dst}, straight, moveCostFunction);
}

Path Path::search(const Area & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	return search(dst.getTiles(), straight, moveCostFunction);
}

Path Path::search(const Path & dst, bool straight, std::function<float(const int3 &, const int3 &)> moveCostFunction) const
{
	assert(dst.dArea == dArea);
	return search(dst.dPath, straight, moveCostFunction);
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

VCMI_LIB_NAMESPACE_END
