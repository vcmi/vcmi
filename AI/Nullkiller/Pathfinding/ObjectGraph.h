/*
* ObjectGraph.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "AINodeStorage.h"
#include "../AIUtility.h"

namespace NKAI
{

class Nullkiller;

struct ObjectLink
{
	float cost = 100000; // some big number
	uint64_t danger = 0;

	void update(float newCost, uint64_t newDanger)
	{
		if(cost > newCost)
		{
			cost = newCost;
			danger = newDanger;
		}
	}
};

struct ObjectNode
{
	ObjectInstanceID objID;
	bool objectExists;
	std::unordered_map<int3, ObjectLink> connections;

	void init(const CGObjectInstance * obj)
	{
		objectExists = true;
		objID = obj->id;
	}
};

class ObjectGraph
{
	std::unordered_map<int3, ObjectNode> nodes;

public:
	void updateGraph(const Nullkiller * ai);
	void addObject(const CGObjectInstance * obj);
	void connectHeroes(const Nullkiller * ai);

	template<typename Func>
	void iterateConnections(const int3 & pos, Func fn)
	{
		for(auto connection : nodes.at(pos).connections)
		{
			fn(connection.first, connection.second);
		}
	}
};

struct GraphPathNode;

class GraphNodeComparer
{
	const std::unordered_map<int3, GraphPathNode> & pathNodes;

public:
	GraphNodeComparer(const std::unordered_map<int3, GraphPathNode> & pathNodes)
		:pathNodes(pathNodes)
	{
	}

	bool operator()(int3 lhs, int3 rhs) const;
};

struct GraphPathNode
{
	const float BAD_COST = 100000;

	int3 previous = int3(-1);
	float cost = BAD_COST;
	uint64_t danger = 0;

	using TFibHeap = boost::heap::fibonacci_heap<int3, boost::heap::compare<GraphNodeComparer>>;

	TFibHeap::handle_type handle;
	bool isInQueue = false;

	bool reachable() const
	{
		return cost < BAD_COST;
	}

	bool tryUpdate(const int3 & pos, const GraphPathNode & prev, const ObjectLink & link)
	{
		auto newCost = prev.cost + link.cost;

		if(newCost < cost)
		{
			previous = pos;
			danger = prev.danger + link.danger;
			cost = newCost;

			return true;
		}

		return false;
	}
};

class GraphPaths
{
	ObjectGraph graph;
	std::unordered_map<int3, GraphPathNode> pathNodes;

public:
	void calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai);
	void addChainInfo(std::vector<AIPath> & paths, int3 tile, const CGHeroInstance * hero, const Nullkiller * ai) const;
	void dumpToLog() const;
};

}
