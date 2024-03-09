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
#include "AIPathfinderConfig.h"
#include "../../../lib/CRandomGenerator.h"
#include "../../../CCallback.h"
#include "../../../lib/mapping/CMap.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/logging/VisualLogger.h"

namespace NKAI
{

class ObjectGraphCalculator
{
private:
	ObjectGraph * target;
	const Nullkiller * ai;

	std::map<const CGHeroInstance *, HeroRole> actors;
	std::map<const CGHeroInstance *, const CGObjectInstance *> actorObjectMap;

	std::vector<std::unique_ptr<CGBoat>> temporaryBoats;
	std::vector<std::unique_ptr<CGHeroInstance>> temporaryActorHeroes;

public:
	ObjectGraphCalculator(ObjectGraph * target, const Nullkiller * ai)
		:ai(ai), target(target)
	{
		for(auto obj : ai->memory->visitableObjs)
		{
			if(obj && obj->isVisitable() && obj->ID != Obj::HERO)
			{
				addObjectActor(obj);
			}
		}

		for(auto town : ai->cb->getTownsInfo())
		{
			addObjectActor(town);
		}

		PathfinderSettings ps;

		ps.mainTurnDistanceLimit = 5;
		ps.scoutTurnDistanceLimit = 1;
		ps.allowBypassObjects = false;

		ai->pathfinder->updatePaths(actors, ps);
	}

	void calculateConnections(const int3 & pos)
	{
		auto guarded = ai->cb->getGuardingCreaturePosition(pos).valid();

		if(guarded)
			return;

		auto paths = ai->pathfinder->getPathInfo(pos);

		for(AIPath & path1 : paths)
		{
			for(AIPath & path2 : paths)
			{
				if(path1.targetHero == path2.targetHero)
					continue;

				auto obj1 = actorObjectMap[path1.targetHero];
				auto obj2 = actorObjectMap[path2.targetHero];

				auto tile1 = cb->getTile(obj1->visitablePos());
				auto tile2 = cb->getTile(obj2->visitablePos());

				if(tile2->isWater() && !tile1->isWater())
				{
					auto linkTile = cb->getTile(pos);

					if(!linkTile->isWater() || obj1->ID != Obj::BOAT || obj1->ID != Obj::SHIPYARD)
						continue;
				}

				auto danger = ai->pathfinder->getStorage()->evaluateDanger(obj2->visitablePos(), path1.targetHero, true);

				auto updated = target->tryAddConnection(
					obj1->visitablePos(),
					obj2->visitablePos(), 
					path1.movementCost() + path2.movementCost(),
					danger);

				if(NKAI_GRAPH_TRACE_LEVEL >= 2 && updated)
				{
					logAi->trace(
						"Connected %s[%s] -> %s[%s] through [%s], cost %2f",
						obj1->getObjectName(), obj1->visitablePos().toString(),
						obj2->getObjectName(), obj2->visitablePos().toString(),
						pos.toString(),
						path1.movementCost() + path2.movementCost());
				}
			}
		}
	}

private:
	void addObjectActor(const CGObjectInstance * obj)
	{
		auto objectActor = temporaryActorHeroes.emplace_back(std::make_unique<CGHeroInstance>(obj->cb)).get();

		CRandomGenerator rng;
		auto visitablePos = obj->visitablePos();

		objectActor->setOwner(ai->playerID); // lets avoid having multiple colors
		objectActor->initHero(rng, static_cast<HeroTypeID>(0));
		objectActor->pos = objectActor->convertFromVisitablePos(visitablePos);
		objectActor->initObj(rng);

		if(cb->getTile(visitablePos)->isWater())
		{
			objectActor->boat = temporaryBoats.emplace_back(std::make_unique<CGBoat>(objectActor->cb)).get();
		}

		actorObjectMap[objectActor] = obj;
		actors[objectActor] = obj->ID == Obj::TOWN || obj->ID == Obj::SHIPYARD ? HeroRole::MAIN : HeroRole::SCOUT;

		target->addObject(obj);
	};
};

bool ObjectGraph::tryAddConnection(
	const int3 & from,
	const int3 & to,
	float cost,
	uint64_t danger)
{
	return nodes[from].connections[to].update(cost, danger);
}

void ObjectGraph::updateGraph(const Nullkiller * ai)
{
	auto cb = ai->cb;

	ObjectGraphCalculator calculator(this, ai);

	foreach_tile_pos(cb.get(), [this, &calculator](const CPlayerSpecificInfoCallback * cb, const int3 & pos)
		{
			if(nodes.find(pos) != nodes.end())
				return;

			calculator.calculateConnections(pos);
		});

	if(NKAI_GRAPH_TRACE_LEVEL >= 1)
		dumpToLog("graph");

}

void ObjectGraph::addObject(const CGObjectInstance * obj)
{
	nodes[obj->visitablePos()].init(obj);
}

void ObjectGraph::removeObject(const CGObjectInstance * obj)
{
	nodes[obj->visitablePos()].objectExists = false;

	if(obj->ID == Obj::BOAT)
	{
		vstd::erase_if(nodes[obj->visitablePos()].connections, [&](const std::pair<int3, ObjectLink> & link) -> bool
			{
				auto tile = cb->getTile(link.first, false);

				return tile && tile->isWater();
			});
	}
}

void ObjectGraph::connectHeroes(const Nullkiller * ai)
{
	for(auto obj : ai->memory->visitableObjs)
	{
		if(obj && obj->ID == Obj::HERO)
		{
			addObject(obj);
		}
	}

	for(auto & node : nodes)
	{
		auto pos = node.first;
		auto paths = ai->pathfinder->getPathInfo(pos);

		for(AIPath & path : paths)
		{
			auto heroPos = path.targetHero->visitablePos();

			nodes[pos].connections[heroPos].update(
				path.movementCost(),
				path.getPathDanger());

			nodes[heroPos].connections[pos].update(
				path.movementCost(),
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
					if(NKAI_GRAPH_TRACE_LEVEL >= 2)
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

bool GraphNodeComparer::operator()(const GraphPathNodePointer & lhs, const GraphPathNodePointer & rhs) const
{
	return pathNodes.at(lhs.coord)[lhs.nodeType].cost > pathNodes.at(rhs.coord)[rhs.nodeType].cost;
}

void GraphPaths::calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai)
{
	graph = *ai->baseGraph;
	graph.connectHeroes(ai);

	visualKey = std::to_string(ai->playerID) + ":" + targetHero->getNameTranslated();
	pathNodes.clear();

	GraphNodeComparer cmp(pathNodes);
	GraphPathNode::TFibHeap pq(cmp);

	pathNodes[targetHero->visitablePos()][GrapthPathNodeType::NORMAL].cost = 0;
	pq.emplace(GraphPathNodePointer(targetHero->visitablePos(), GrapthPathNodeType::NORMAL));

	while(!pq.empty())
	{
		GraphPathNodePointer pos = pq.top();
		pq.pop();

		auto & node = getNode(pos);

		node.isInQueue = false;

		graph.iterateConnections(pos.coord, [&](int3 target, ObjectLink o)
			{
				auto targetNodeType = o.danger ? GrapthPathNodeType::BATTLE : pos.nodeType;
				auto targetPointer = GraphPathNodePointer(target, targetNodeType);
				auto & targetNode = getNode(targetPointer);

				if(targetNode.tryUpdate(pos, node, o))
				{
					if(graph.getNode(target).objTypeID == Obj::HERO)
						return;

					if(targetNode.isInQueue)
					{
						pq.increase(targetNode.handle);
					}
					else
					{
						targetNode.handle = pq.emplace(targetPointer);
						targetNode.isInQueue = true;
					}
				}
			});
	}
}

void GraphPaths::dumpToLog() const
{
	logVisual->updateWithLock(visualKey, [&](IVisualLogBuilder & logBuilder)
		{
			for(auto & tile : pathNodes)
			{
				for(auto & node : tile.second)
				{
					if(!node.previous.valid())
						continue;

					if(NKAI_GRAPH_TRACE_LEVEL >= 2)
					{
						logAi->trace(
							"%s -> %s: %f !%d",
							node.previous.coord.toString(),
							tile.first.toString(),
							node.cost,
							node.danger);
					}

					logBuilder.addLine(node.previous.coord, tile.first);
				}
			}
		});
}

bool GraphPathNode::tryUpdate(const GraphPathNodePointer & pos, const GraphPathNode & prev, const ObjectLink & link)
{
	auto newCost = prev.cost + link.cost;

	if(newCost < cost)
	{
		if(nodeType < pos.nodeType)
		{
			logAi->error("Linking error");
		}

		previous = pos;
		danger = prev.danger + link.danger;
		cost = newCost;

		return true;
	}

	return false;
}

void GraphPaths::addChainInfo(std::vector<AIPath> & paths, int3 tile, const CGHeroInstance * hero, const Nullkiller * ai) const
{
	auto nodes = pathNodes.find(tile);

	if(nodes == pathNodes.end())
		return;

	for(auto & node : nodes->second)
	{
		if(!node.reachable())
			continue;

		std::vector<int3> tilesToPass;

		uint64_t danger = node.danger;
		float cost = node.cost;
		bool allowBattle = false;

		auto current = GraphPathNodePointer(nodes->first, node.nodeType);

		while(true)
		{
			auto currentTile = pathNodes.find(current.coord);

			if(currentTile == pathNodes.end())
				break;

			auto currentNode = currentTile->second[current.nodeType];

			if(!currentNode.previous.valid())
				break;

			allowBattle = allowBattle || currentNode.nodeType == GrapthPathNodeType::BATTLE;
			vstd::amax(danger, currentNode.danger);
			vstd::amax(cost, currentNode.cost);

			tilesToPass.push_back(current.coord);

			if(currentNode.cost < 2.0f)
				break;

			current = currentNode.previous;
		}

		if(tilesToPass.empty())
			continue;

		auto entryPaths = ai->pathfinder->getPathInfo(tilesToPass.back());

		for(auto & path : entryPaths)
		{
			if(path.targetHero != hero)
				continue;

			for(auto graphTile = tilesToPass.rbegin(); graphTile != tilesToPass.rend(); graphTile++)
			{
				AIPathNodeInfo n;

				n.coord = *graphTile;
				n.cost = cost;
				n.turns = static_cast<ui8>(cost) + 1; // just in case lets select worst scenario
				n.danger = danger;
				n.targetHero = hero;
				n.parentIndex = 0;

				for(auto & node : path.nodes)
				{
					node.parentIndex++;
				}

				path.nodes.insert(path.nodes.begin(), n);
			}

			path.armyLoss += ai->pathfinder->getStorage()->evaluateArmyLoss(path.targetHero, path.heroArmy->getArmyStrength(), danger);
			path.targetObjectDanger = ai->pathfinder->getStorage()->evaluateDanger(tile, path.targetHero, !allowBattle);
			path.targetObjectArmyLoss = ai->pathfinder->getStorage()->evaluateArmyLoss(path.targetHero, path.heroArmy->getArmyStrength(), path.targetObjectDanger);

			paths.push_back(path);
		}
	}
}

}
