/*
* GraphPaths.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "ObjectGraph.h"

namespace NKAI
{

class Nullkiller;

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
		return coord.isValid();
	}
};

using GraphNodeStorage = std::unordered_map<int3, GraphPathNode[GrapthPathNodeType::LAST]>;

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
	uint64_t linkDanger = 0;
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
	void calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai, uint8_t scanDepth);
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
