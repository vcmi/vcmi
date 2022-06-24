/*
 * CRmgPath.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "CRmgPath.h"
#include <boost/heap/priority_queue.hpp> //A*

using namespace Rmg;

//A* priority queue
typedef std::pair<int3, float> TDistance;
struct NodeComparer
{
	bool operator()(const TDistance & lhs, const TDistance & rhs) const
	{
		return (rhs.second < lhs.second);
	}
};
//boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue();
boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue()
{
	return boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>>();
}

Path::Path(const Area & area): dArea(area)
{
}

Path::Path(const Area & area, const int3 & src): dArea(area)
{
	dPath.add(src);
}

bool Path::valid() const
{
	return !dPath.empty();
}

Path Path::search(const Tileset & dst, bool straigth) const
{
	//A* algorithm taken from Wiki http://en.wikipedia.org/wiki/A*_search_algorithm
	Path result(dArea);
	if(dst.empty())
		return result;
	
	result.dPath.assign(dst);
	int3 src = result.dPath.nearest(dArea);
	
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
			auto foo = [this, &open, &closed, &cameFrom, &currentNode, &distances](const int3& pos) -> void
			{
				if(closed.count(pos))
					return;
				
				if(!dArea.contains(pos))
					return;
				
				float movementCost = 0;
				
				if(dPath.contains(pos))
					movementCost = 1;
				else
					movementCost = 2;
				
				float distance = distances[currentNode] + movementCost; //we prefer to use already free paths
				int bestDistanceSoFar = std::numeric_limits<int>::max(); //FIXME: boost::limits
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
			if (straigth)
				neighbors = { { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0) } };
			for(auto & i : neighbors)
			{
				foo(currentNode + i);
			}
		}
		
	}
	
	return result;
}

Path Path::search(const int3 & dst, bool straight) const
{
	return search(Tileset{dst}, straight);
}

Path Path::search(const Area & dst, bool straight) const
{
	return search(dst.getTiles(), straight);
}

Path Path::search(const Path & dst, bool straight) const
{
	assert(dst.dArea == dArea);
	return search(dst.dPath, straight);
}

bool Path::connect(const int3 & path)
{
	dPath.add(path);
	if(dPath.connected())
	{
		return true;
	}
	return false;
}

bool Path::connect(const Tileset & path)
{
	Area a(path);
	dPath.unite(a);
	if(dPath.connected())
	{
		return true;
	}
	return false;
}

bool Path::connect(const Path & path)
{
	//assert(path.dArea == dArea);
	auto newPath = path.dPath + dPath;
	if(newPath.connected())
	{
		dPath = newPath;
		return true;
	}
	return false;
}

const Area & Path::getPathArea() const
{
	return dPath;
}
