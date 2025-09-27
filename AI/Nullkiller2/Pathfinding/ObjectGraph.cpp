/*
* ObjectGraph.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ObjectGraph.h"
#include "ObjectGraphCalculator.h"
#include "AIPathfinderConfig.h"
#include "../../../lib/CRandomGenerator.h"
#include "../../../lib/mapping/CMap.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/logging/VisualLogger.h"
#include "Actions/QuestAction.h"
#include "../pforeach.h"
#include "Actions/BoatActions.h"

namespace NK2AI
{

bool ObjectGraph::tryAddConnection(
	const int3 & from,
	const int3 & to,
	float cost,
	uint64_t danger)
{
	auto result =  nodes[from].connections[to].update(cost, danger);
	auto & connection = nodes[from].connections[to];

	if(result && isVirtualBoat(to) && !connection.specialAction)
	{
		connection.specialAction = std::make_shared<AIPathfinding::BuildBoatActionFactory>(virtualBoats[to]);
	}

	return result;
}

void ObjectGraph::removeConnection(const int3 & from, const int3 & to)
{
	nodes[from].connections.erase(to);
}

void ObjectGraph::updateGraph(const Nullkiller * aiNk)
{
	ObjectGraphCalculator calculator(this, aiNk);

	calculator.setGraphObjects();
	calculator.calculateConnections();
	calculator.addMinimalDistanceJunctions();
	calculator.calculateConnections();

	if(NK2AI_GRAPH_TRACE_LEVEL >= 1)
		dumpToLog("graph");
}

void ObjectGraph::addObject(const CGObjectInstance * obj)
{
	if(!hasNodeAt(obj->visitablePos()))
		nodes[obj->visitablePos()].init(obj);
}

void ObjectGraph::addVirtualBoat(const int3 & pos, const CGObjectInstance * shipyard)
{
	if(!isVirtualBoat(pos))
	{
		virtualBoats[pos] = shipyard->id;
	}
}

void ObjectGraph::registerJunction(const int3 & pos)
{
	if(!hasNodeAt(pos))
		nodes[pos].initJunction();

}

void ObjectGraph::removeObject(const CGObjectInstance * obj, CCallback & cc)
{
	nodes[obj->visitablePos()].objectExists = false;

	if(obj->ID == Obj::BOAT && !isVirtualBoat(obj->visitablePos()))
	{
		vstd::erase_if(nodes[obj->visitablePos()].connections, [&](const std::pair<int3, ObjectLink> & link) -> bool
			{
				const auto tile = cc.getTile(link.first, false);
				return tile && tile->isWater();
			});
	}
}

void ObjectGraph::connectHeroes(const Nullkiller * aiNk)
{
	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = aiNk->cc->getObjInstance(objId);
		if(obj && obj->ID == Obj::HERO)
		{
			addObject(obj);
		}
	}

	for(auto & node : nodes)
	{
		auto pos = node.first;
		auto paths = aiNk->pathfinder->getPathInfo(pos);

		for(AIPath & path : paths)
		{
			if(path.getFirstBlockedAction())
				continue;

			auto heroPos = path.targetHero->visitablePos();

			nodes[pos].connections[heroPos].update(
				std::max(0.0f, path.movementCost()),
				path.getPathDanger());

			nodes[heroPos].connections[pos].update(
				std::max(0.0f, path.movementCost()),
				path.getPathDanger());
		}
	}
}

void ObjectGraph::dumpToLog(std::string visualKey) const
{
	logVisual->updateWithLock(visualKey, [&](IVisualLogBuilder & logBuilder)
		{
			for(auto & tile : nodes)
			{
				for(auto & node : tile.second.connections)
				{
					if(NK2AI_GRAPH_TRACE_LEVEL >= 2)
					{
						logAi->trace(
							"%s -> %s: %f !%d",
							node.first.toString(),
							tile.first.toString(),
							node.second.cost,
							node.second.danger);
					}

					logBuilder.addLine(tile.first, node.first);
				}
			}
		});
}

}
