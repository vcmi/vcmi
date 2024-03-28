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
#include "Actions/QuestAction.h"
#include "../pforeach.h"
#include "Actions/BoatActions.h"

namespace NKAI
{

struct ConnectionCostInfo
{
	float totalCost = 0;
	float avg = 0;
	int connectionsCount = 0;
};

class ObjectGraphCalculator
{
private:
	ObjectGraph * target;
	const Nullkiller * ai;
	std::mutex syncLock;

	std::map<const CGHeroInstance *, HeroRole> actors;
	std::map<const CGHeroInstance *, const CGObjectInstance *> actorObjectMap;

	std::vector<std::unique_ptr<CGBoat>> temporaryBoats;
	std::vector<std::unique_ptr<CGHeroInstance>> temporaryActorHeroes;

public:
	ObjectGraphCalculator(ObjectGraph * target, const Nullkiller * ai)
		:ai(ai), target(target), syncLock()
	{
	}

	void setGraphObjects()
	{
		for(auto obj : ai->memory->visitableObjs)
		{
			if(obj && obj->isVisitable() && obj->ID != Obj::HERO && obj->ID != Obj::EVENT)
			{
				addObjectActor(obj);
			}
		}

		for(auto town : ai->cb->getTownsInfo())
		{
			addObjectActor(town);
		}
	}

	void calculateConnections()
	{
		updatePaths();

		std::vector<AIPath> pathCache;

		foreach_tile_pos(ai->cb.get(), [this, &pathCache](const CPlayerSpecificInfoCallback * cb, const int3 & pos)
			{
				calculateConnections(pos, pathCache);
			});

		removeExtraConnections();
	}

	float getNeighborConnectionsCost(const int3 & pos, std::vector<AIPath> & pathCache)
	{
		float neighborCost = std::numeric_limits<float>::max();

		if(NKAI_GRAPH_TRACE_LEVEL >= 2)
		{
			logAi->trace("Checking junction %s", pos.toString());
		}

		foreach_neighbour(
			ai->cb.get(),
			pos,
			[this, &neighborCost, &pathCache](const CPlayerSpecificInfoCallback * cb, const int3 & neighbor)
			{
				ai->pathfinder->calculatePathInfo(pathCache, neighbor);

				auto costTotal = this->getConnectionsCost(pathCache);

				if(costTotal.connectionsCount > 2 && costTotal.avg < neighborCost)
				{
					neighborCost = costTotal.avg;

					if(NKAI_GRAPH_TRACE_LEVEL >= 2)
					{
						logAi->trace("Better node found at %s", neighbor.toString());
					}
				}
			});

		return neighborCost;
	}

	void addMinimalDistanceJunctions()
	{
		pforeachTilePaths(ai->cb->getMapSize(), ai, [this](const int3 & pos, std::vector<AIPath> & paths)
			{
				if(target->hasNodeAt(pos))
					return;

				if(ai->cb->getGuardingCreaturePosition(pos).valid())
					return;

				ConnectionCostInfo currentCost = getConnectionsCost(paths);

				if(currentCost.connectionsCount <= 2)
					return;

				float neighborCost = getNeighborConnectionsCost(pos, paths);

				if(currentCost.avg < neighborCost)
				{
					addJunctionActor(pos);
				}
			});
	}

private:
	void updatePaths()
	{
		PathfinderSettings ps;

		ps.mainTurnDistanceLimit = 5;
		ps.scoutTurnDistanceLimit = 1;
		ps.allowBypassObjects = false;

		ai->pathfinder->updatePaths(actors, ps);
	}

	void calculateConnections(const int3 & pos, std::vector<AIPath> & pathCache)
	{
		if(target->hasNodeAt(pos))
		{
			foreach_neighbour(
				ai->cb.get(),
				pos,
				[this, &pos, &pathCache](const CPlayerSpecificInfoCallback * cb, const int3 & neighbor)
				{
					if(target->hasNodeAt(neighbor))
					{
						ai->pathfinder->calculatePathInfo(pathCache, neighbor);

						for(auto & path : pathCache)
						{
							if(pos == path.targetHero->visitablePos())
							{
								target->tryAddConnection(pos, neighbor, path.movementCost(), path.getTotalDanger());
							}
						}
					}
				});

			auto obj = ai->cb->getTopObj(pos);

			if((obj && obj->ID == Obj::BOAT) || target->isVirtualBoat(pos))
			{
				ai->pathfinder->calculatePathInfo(pathCache, pos);

				for(AIPath & path : pathCache)
				{
					auto from = path.targetHero->visitablePos();
					auto fromObj = actorObjectMap[path.targetHero];

					auto danger = ai->pathfinder->getStorage()->evaluateDanger(pos, path.targetHero, true);
					auto updated = target->tryAddConnection(
						from,
						pos,
						path.movementCost(),
						danger);

					if(NKAI_GRAPH_TRACE_LEVEL >= 2 && updated)
					{
						logAi->trace(
							"Connected %s[%s] -> %s[%s] through [%s], cost %2f",
							fromObj ? fromObj->getObjectName() : "J", from.toString(),
							"Boat", pos.toString(),
							pos.toString(),
							path.movementCost());
					}
				}
			}

			return;
		}

		auto guardPos = ai->cb->getGuardingCreaturePosition(pos);
		
		ai->pathfinder->calculatePathInfo(pathCache, pos);

		for(AIPath & path1 : pathCache)
		{
			for(AIPath & path2 : pathCache)
			{
				if(path1.targetHero == path2.targetHero)
					continue;

				auto pos1 = path1.targetHero->visitablePos();
				auto pos2 = path2.targetHero->visitablePos();

				if(guardPos.valid() && guardPos != pos1 && guardPos != pos2)
					continue;

				auto obj1 = actorObjectMap[path1.targetHero];
				auto obj2 = actorObjectMap[path2.targetHero];

				auto tile1 = cb->getTile(pos1);
				auto tile2 = cb->getTile(pos2);

				if(tile2->isWater() && !tile1->isWater())
				{
					if(!cb->getTile(pos)->isWater())
						continue;

					auto startingObjIsBoat = (obj1 && obj1->ID == Obj::BOAT) || target->isVirtualBoat(pos1);

					if(!startingObjIsBoat)
						continue;
				}

				auto danger = ai->pathfinder->getStorage()->evaluateDanger(pos2, path1.targetHero, true);

				auto updated = target->tryAddConnection(
					pos1,
					pos2,
					path1.movementCost() + path2.movementCost(),
					danger);

				if(NKAI_GRAPH_TRACE_LEVEL >= 2 && updated)
				{
					logAi->trace(
						"Connected %s[%s] -> %s[%s] through [%s], cost %2f",
						obj1 ? obj1->getObjectName() : "J", pos1.toString(),
						obj2 ? obj2->getObjectName() : "J", pos2.toString(),
						pos.toString(),
						path1.movementCost() + path2.movementCost());
				}
			}
		}
	}

	bool isExtraConnection(float direct, float side1, float side2) const
	{
		float sideRatio = (side1 + side2) / direct;

		return sideRatio < 1.25f && direct > side1 && direct > side2;
	}

	void removeExtraConnections()
	{
		std::vector<std::pair<int3, int3>> connectionsToRemove;

		for(auto & actor : temporaryActorHeroes)
		{
			auto pos = actor->visitablePos();
			auto & currentNode = target->getNode(pos);

			target->iterateConnections(pos, [this, &pos, &connectionsToRemove, &currentNode](int3 n1, ObjectLink o1)
				{
					target->iterateConnections(n1, [&pos, &o1, &currentNode, &connectionsToRemove, this](int3 n2, ObjectLink o2)
						{
							auto direct = currentNode.connections.find(n2);

							if(direct != currentNode.connections.end() && isExtraConnection(direct->second.cost, o1.cost, o2.cost))
							{
								connectionsToRemove.push_back({pos, n2});
							}
						});
				});
		}

		vstd::removeDuplicates(connectionsToRemove);

		for(auto & c : connectionsToRemove)
		{
			target->removeConnection(c.first, c.second);

			if(NKAI_GRAPH_TRACE_LEVEL >= 2)
			{
				logAi->trace("Remove ineffective connection %s->%s", c.first.toString(), c.second.toString());
			}
		}
	}

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

		assert(objectActor->visitablePos() == visitablePos);

		actorObjectMap[objectActor] = obj;
		actors[objectActor] = obj->ID == Obj::TOWN || obj->ID == Obj::BOAT ? HeroRole::MAIN : HeroRole::SCOUT;

		target->addObject(obj);

		auto shipyard = dynamic_cast<const IShipyard *>(obj);
		
		if(shipyard && shipyard->bestLocation().valid())
		{
			int3 virtualBoat = shipyard->bestLocation();
			
			addJunctionActor(virtualBoat, true);
			target->addVirtualBoat(virtualBoat, obj);
		}
	}

	void addJunctionActor(const int3 & visitablePos, bool isVirtualBoat = false)
	{
		std::lock_guard<std::mutex> lock(syncLock);

		auto internalCb = temporaryActorHeroes.front()->cb;
		auto objectActor = temporaryActorHeroes.emplace_back(std::make_unique<CGHeroInstance>(internalCb)).get();

		CRandomGenerator rng;

		objectActor->setOwner(ai->playerID); // lets avoid having multiple colors
		objectActor->initHero(rng, static_cast<HeroTypeID>(0));
		objectActor->pos = objectActor->convertFromVisitablePos(visitablePos);
		objectActor->initObj(rng);

		if(isVirtualBoat || ai->cb->getTile(visitablePos)->isWater())
		{
			objectActor->boat = temporaryBoats.emplace_back(std::make_unique<CGBoat>(objectActor->cb)).get();
		}

		assert(objectActor->visitablePos() == visitablePos);

		actorObjectMap[objectActor] = nullptr;
		actors[objectActor] = isVirtualBoat ? HeroRole::MAIN : HeroRole::SCOUT;

		target->registerJunction(visitablePos);
	}

	ConnectionCostInfo getConnectionsCost(std::vector<AIPath> & paths) const
	{
		std::map<int3, float> costs;

		for(auto & path : paths)
		{
			auto fromPos = path.targetHero->visitablePos();
			auto cost = costs.find(fromPos);
			
			if(cost == costs.end())
			{
				costs.emplace(fromPos, path.movementCost());
			}
			else
			{
				if(path.movementCost() < cost->second)
				{
					costs[fromPos] = path.movementCost();
				}
			}
		}

		ConnectionCostInfo result;

		for(auto & cost : costs)
		{
			result.totalCost += cost.second;
			result.connectionsCount++;
		}

		if(result.connectionsCount)
		{
			result.avg = result.totalCost / result.connectionsCount;
		}

		return result;
	}
};

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

void ObjectGraph::updateGraph(const Nullkiller * ai)
{
	auto cb = ai->cb;

	ObjectGraphCalculator calculator(this, ai);

	calculator.setGraphObjects();
	calculator.calculateConnections();
	calculator.addMinimalDistanceJunctions();
	calculator.calculateConnections();

	if(NKAI_GRAPH_TRACE_LEVEL >= 1)
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

void ObjectGraph::removeObject(const CGObjectInstance * obj)
{
	nodes[obj->visitablePos()].objectExists = false;

	if(obj->ID == Obj::BOAT && !isVirtualBoat(obj->visitablePos()))
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
			if(path.getFirstBlockedAction())
				continue;

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

GraphPaths::GraphPaths()
	: visualKey(""), graph(), pathNodes()
{
}

std::shared_ptr<SpecialAction> getCompositeAction(
	const Nullkiller * ai,
	std::shared_ptr<ISpecialActionFactory> linkActionFactory,
	std::shared_ptr<SpecialAction> transitionAction)
{
	if(!linkActionFactory)
		return transitionAction;

	auto linkAction = linkActionFactory->create(ai);

	if(!transitionAction)
		return linkAction;

	std::vector<std::shared_ptr<const SpecialAction>> actionsArray = {
		transitionAction,
		linkAction
	};

	return std::make_shared<CompositeAction>(actionsArray);
}

void GraphPaths::calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai)
{
	graph.copyFrom(*ai->baseGraph);
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

		auto & node = getOrCreateNode(pos);
		std::shared_ptr<SpecialAction> transitionAction;

		if(node.obj)
		{
			if(node.obj->ID == Obj::QUEST_GUARD
				|| node.obj->ID == Obj::BORDERGUARD
				|| node.obj->ID == Obj::BORDER_GATE)
			{
				auto questObj = dynamic_cast<const IQuestObject *>(node.obj);
				auto questInfo = QuestInfo(questObj->quest, node.obj, pos.coord);

				if(node.obj->ID == Obj::QUEST_GUARD
					&& questObj->quest->mission == Rewardable::Limiter{}
					&& questObj->quest->killTarget == ObjectInstanceID::NONE)
				{
					continue;
				}

				auto questAction = std::make_shared<AIPathfinding::QuestAction>(questInfo);

				if(!questAction->canAct(targetHero))
				{
					transitionAction = questAction;
				}
			}
		}

		node.isInQueue = false;

		graph.iterateConnections(pos.coord, [this, ai, &pos, &node, &transitionAction, &pq](int3 target, const ObjectLink & o)
			{
				auto compositeAction = getCompositeAction(ai, o.specialAction, transitionAction);
				auto targetNodeType = o.danger || compositeAction ? GrapthPathNodeType::BATTLE : pos.nodeType;
				auto targetPointer = GraphPathNodePointer(target, targetNodeType);
				auto & targetNode = getOrCreateNode(targetPointer);

				if(targetNode.tryUpdate(pos, node, o))
				{
					targetNode.specialAction = compositeAction;

					auto targetGraphNode = graph.getNode(target);

					if(targetGraphNode.objID.hasValue())
					{
						targetNode.obj = ai->cb->getObj(targetGraphNode.objID, false);

						if(targetNode.obj && targetNode.obj->ID == Obj::HERO)
							return;
					}

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

		std::vector<GraphPathNodePointer> tilesToPass;

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

			tilesToPass.push_back(current);

			if(currentNode.cost < 2.0f)
				break;

			current = currentNode.previous;
		}

		if(tilesToPass.empty())
			continue;

		auto entryPaths = ai->pathfinder->getPathInfo(tilesToPass.back().coord);

		for(auto & path : entryPaths)
		{
			if(path.targetHero != hero)
				continue;

			for(auto graphTile = tilesToPass.rbegin(); graphTile != tilesToPass.rend(); graphTile++)
			{
				AIPathNodeInfo n;

				n.coord = graphTile->coord;
				n.cost = cost;
				n.turns = static_cast<ui8>(cost) + 1; // just in case lets select worst scenario
				n.danger = danger;
				n.targetHero = hero;
				n.parentIndex = -1;
				n.specialAction = getNode(*graphTile).specialAction;

				if(n.specialAction)
				{
					n.actionIsBlocked = !n.specialAction->canAct(n);
				}

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

void GraphPaths::quickAddChainInfoWithBlocker(std::vector<AIPath> & paths, int3 tile, const CGHeroInstance * hero, const Nullkiller * ai) const
{
	auto nodes = pathNodes.find(tile);

	if(nodes == pathNodes.end())
		return;

	for(auto & targetNode : nodes->second)
	{
		if(!targetNode.reachable())
			continue;

		std::vector<GraphPathNodePointer> tilesToPass;

		uint64_t danger = targetNode.danger;
		float cost = targetNode.cost;
		bool allowBattle = false;

		auto current = GraphPathNodePointer(nodes->first, targetNode.nodeType);

		while(true)
		{
			auto currentTile = pathNodes.find(current.coord);

			if(currentTile == pathNodes.end())
				break;

			auto currentNode = currentTile->second[current.nodeType];

			allowBattle = allowBattle || currentNode.nodeType == GrapthPathNodeType::BATTLE;
			vstd::amax(danger, currentNode.danger);
			vstd::amax(cost, currentNode.cost);

			tilesToPass.push_back(current);

			if(currentNode.cost < 2.0f)
				break;

			current = currentNode.previous;
		}
		
		if(tilesToPass.empty())
			continue;

		auto entryPaths = ai->pathfinder->getPathInfo(tilesToPass.back().coord);

		for(auto & entryPath : entryPaths)
		{
			if(entryPath.targetHero != hero)
				continue;

			auto & path = paths.emplace_back();

			path.targetHero = entryPath.targetHero;
			path.heroArmy = entryPath.heroArmy;
			path.exchangeCount = entryPath.exchangeCount;
			path.armyLoss = entryPath.armyLoss + ai->pathfinder->getStorage()->evaluateArmyLoss(path.targetHero, path.heroArmy->getArmyStrength(), danger);
			path.targetObjectDanger = ai->pathfinder->getStorage()->evaluateDanger(tile, path.targetHero, !allowBattle);
			path.targetObjectArmyLoss = ai->pathfinder->getStorage()->evaluateArmyLoss(path.targetHero, path.heroArmy->getArmyStrength(), path.targetObjectDanger);

			AIPathNodeInfo n;

			n.targetHero = hero;
			n.parentIndex = -1;

			// final node
			n.coord = tile;
			n.cost = targetNode.cost;
			n.danger = targetNode.danger;
			n.parentIndex = path.nodes.size();
			path.nodes.push_back(n);

			for(auto entryNode = entryPath.nodes.rbegin(); entryNode != entryPath.nodes.rend(); entryNode++)
			{
				auto blocker = ai->objectClusterizer->getBlocker(*entryNode);

				if(blocker)
				{
					// blocker node
					path.nodes.push_back(*entryNode);
					path.nodes.back().parentIndex = path.nodes.size() - 1;
					break;
				}
			}
			
			if(path.nodes.size() > 1)
				continue;

			for(auto graphTile = tilesToPass.rbegin(); graphTile != tilesToPass.rend(); graphTile++)
			{
				auto & node = getNode(*graphTile);

				n.coord = graphTile->coord;
				n.cost = node.cost;
				n.turns = static_cast<ui8>(node.cost);
				n.danger = node.danger;
				n.specialAction = node.specialAction;
				n.parentIndex = path.nodes.size();

				if(n.specialAction)
				{
					n.actionIsBlocked = !n.specialAction->canAct(n);
				}

				auto blocker = ai->objectClusterizer->getBlocker(n);

				if(!blocker)
					continue;

				// blocker node
				path.nodes.push_back(n);
				break;
			}
		}
	}
}

}
