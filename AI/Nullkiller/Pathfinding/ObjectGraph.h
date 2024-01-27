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

enum GrapthPathNodeType
{
	NORMAL,

	BATTLE,

	LAST
};

struct GraphPathNodePointer
{
	int3 coord = int3(-1);
	GrapthPathNodeType nodeType = GrapthPathNodeType::NORMAL;

	GraphPathNodePointer() = default;

	GraphPathNodePointer(int3 coord, GrapthPathNodeType type)
		:coord(coord), nodeType(type)
	{ }

	bool valid() const
	{
		return coord.valid();
	}
};

typedef std::unordered_map<int3, GraphPathNode[GrapthPathNodeType::LAST]> GraphNodeStorage;

class GraphNodeComparer
{
	const GraphNodeStorage & pathNodes;

public:
	GraphNodeComparer(const GraphNodeStorage & pathNodes)
		:pathNodes(pathNodes)
	{
	}

	bool operator()(const GraphPathNodePointer & lhs, const GraphPathNodePointer & rhs) const;
};

struct GraphPathNode
{
	const float BAD_COST = 100000;

	GrapthPathNodeType nodeType = GrapthPathNodeType::NORMAL;
	GraphPathNodePointer previous;
	float cost = BAD_COST;
	uint64_t danger = 0;

	using TFibHeap = boost::heap::fibonacci_heap<GraphPathNodePointer, boost::heap::compare<GraphNodeComparer>>;

	TFibHeap::handle_type handle;
	bool isInQueue = false;

	bool reachable() const
	{
		return cost < BAD_COST;
	}

	bool tryUpdate(const GraphPathNodePointer & pos, const GraphPathNode & prev, const ObjectLink & link);
};

class GraphPaths
{
	ObjectGraph graph;
	GraphNodeStorage pathNodes;

public:
	void calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai);
	void addChainInfo(std::vector<AIPath> & paths, int3 tile, const CGHeroInstance * hero, const Nullkiller * ai) const;
	void dumpToLog() const;

private:
	GraphPathNode & getNode(const GraphPathNodePointer & pos)
	{
		return pathNodes[pos.coord][pos.nodeType];
	}
};

}
