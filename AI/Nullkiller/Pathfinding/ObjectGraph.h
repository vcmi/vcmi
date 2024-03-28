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
	std::shared_ptr<ISpecialActionFactory> specialAction;

	bool update(float newCost, uint64_t newDanger)
	{
		if(cost > newCost)
		{
			cost = newCost;
			danger = newDanger;

			return true;
		}

		return false;
	}
};

struct ObjectNode
{
	ObjectInstanceID objID;
	MapObjectID objTypeID;
	bool objectExists;
	std::unordered_map<int3, ObjectLink> connections;

	void init(const CGObjectInstance * obj)
	{
		objectExists = true;
		objID = obj->id;
		objTypeID = obj->ID;
	}

	void initJunction()
	{
		objectExists = false;
		objID = ObjectInstanceID();
		objTypeID = Obj();
	}
};

class ObjectGraph
{
	std::unordered_map<int3, ObjectNode> nodes;
	std::unordered_map<int3, ObjectInstanceID> virtualBoats;

public:
	ObjectGraph()
		:nodes(), virtualBoats()
	{
	}

	void updateGraph(const Nullkiller * ai);
	void addObject(const CGObjectInstance * obj);
	void registerJunction(const int3 & pos);
	void addVirtualBoat(const int3 & pos, const CGObjectInstance * shipyard);
	void connectHeroes(const Nullkiller * ai);
	void removeObject(const CGObjectInstance * obj);
	bool tryAddConnection(const int3 & from, const int3 & to, float cost, uint64_t danger);
	void removeConnection(const int3 & from, const int3 & to);
	void dumpToLog(std::string visualKey) const;

	bool isVirtualBoat(const int3 & tile) const
	{
		return vstd::contains(virtualBoats, tile);
	}

	void copyFrom(const ObjectGraph & other)
	{
		nodes = other.nodes;
		virtualBoats = other.virtualBoats;
	}

	template<typename Func>
	void iterateConnections(const int3 & pos, Func fn)
	{
		for(auto & connection : nodes.at(pos).connections)
		{
			fn(connection.first, connection.second);
		}
	}

	const ObjectNode & getNode(int3 tile) const
	{
		return nodes.at(tile);
	}

	bool hasNodeAt(const int3 & tile) const
	{
		return vstd::contains(nodes, tile);
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
	const CGObjectInstance * obj = nullptr;
	std::shared_ptr<SpecialAction> specialAction;

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
	std::string visualKey;

public:
	GraphPaths();
	void calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai);
	void addChainInfo(std::vector<AIPath> & paths, int3 tile, const CGHeroInstance * hero, const Nullkiller * ai) const;
	void quickAddChainInfoWithBlocker(std::vector<AIPath> & paths, int3 tile, const CGHeroInstance * hero, const Nullkiller * ai) const;
	void dumpToLog() const;

private:
	GraphPathNode & getOrCreateNode(const GraphPathNodePointer & pos)
	{
		auto & node = pathNodes[pos.coord][pos.nodeType];

		node.nodeType = pos.nodeType;

		return node;
	}

	const GraphPathNode & getNode(const GraphPathNodePointer & pos) const
	{
		auto & node = pathNodes.at(pos.coord)[pos.nodeType];

		return node;
	}
};

}
