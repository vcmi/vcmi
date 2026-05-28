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

namespace NK2AI
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

	void updateGraph(const Nullkiller * aiNk);
	void addObject(const CGObjectInstance * obj);
	void registerJunction(const int3 & pos);
	void addVirtualBoat(const int3 & pos, const CGObjectInstance * shipyard);
	void connectHeroes(const Nullkiller * aiNk);
	void removeObject(const CGObjectInstance * obj, CCallback & cc);
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

}
