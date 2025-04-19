/*
* GraphPaths.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "GraphPaths.h"
#include "AIPathfinderConfig.h"
#include "../../../lib/CRandomGenerator.h"
#include "../../../CCallback.h"
#include "../../../lib/mapObjects/CQuest.h"
#include "../../../lib/mapping/CMap.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/logging/VisualLogger.h"
#include "Actions/QuestAction.h"
#include "../pforeach.h"
#include "Actions/BoatActions.h"

namespace NKAI
{

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

void GraphPaths::calculatePaths(const CGHeroInstance * targetHero, const Nullkiller * ai, uint8_t scanDepth)
{
	graph.copyFrom(*ai->baseGraph);
	graph.connectHeroes(ai);

	visualKey = std::to_string(ai->playerID.getNum()) + ":" + targetHero->getNameTranslated();
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
				auto questInfo = QuestInfo(node.obj->id);

				if(node.obj->ID == Obj::QUEST_GUARD
					&& questObj->getQuest().mission == Rewardable::Limiter{}
					&& questObj->getQuest().killTarget == ObjectInstanceID::NONE)
				{
					continue;
				}

				auto questAction = std::make_shared<AIPathfinding::QuestAction>(questInfo);

				if(!questAction->canAct(ai, targetHero))
				{
					transitionAction = questAction;
				}
			}
		}

		node.isInQueue = false;

		graph.iterateConnections(pos.coord, [this, ai, &pos, &node, &transitionAction, &pq, scanDepth](int3 target, const ObjectLink & o)
			{
				auto compositeAction = getCompositeAction(ai, o.specialAction, transitionAction);
				auto targetNodeType = o.danger || compositeAction ? GrapthPathNodeType::BATTLE : pos.nodeType;
				auto targetPointer = GraphPathNodePointer(target, targetNodeType);
				auto & targetNode = getOrCreateNode(targetPointer);

				if(targetNode.tryUpdate(pos, node, o))
				{
					if(targetNode.cost > scanDepth)
					{
						return;
					}

					targetNode.specialAction = compositeAction;

					const auto & targetGraphNode = graph.getNode(target);

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
							node.linkDanger);
					}

					logBuilder.addLine(node.previous.coord, tile.first);
				}
			}
		});
}

bool GraphPathNode::tryUpdate(
	const GraphPathNodePointer & pos,
	const GraphPathNode & prev,
	const ObjectLink & link)
{
	auto newCost = prev.cost + link.cost;

	if(newCost < cost)
	{
		previous = pos;
		linkDanger = link.danger;
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

		uint64_t danger = node.linkDanger;
		float cost = node.cost;
		bool allowBattle = false;

		auto current = GraphPathNodePointer(nodes->first, node.nodeType);

		while(true)
		{
			auto currentTile = pathNodes.find(current.coord);

			if(currentTile == pathNodes.end())
				break;

			auto & currentNode = currentTile->second[current.nodeType];

			if(!currentNode.previous.valid())
				break;

			allowBattle = allowBattle || currentNode.nodeType == GrapthPathNodeType::BATTLE;
			vstd::amax(danger, currentNode.linkDanger);
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

			uint64_t loss = 0;
			uint64_t strength = getHeroArmyStrengthWithCommander(path.targetHero, path.heroArmy);

			for(auto graphTile = ++tilesToPass.rbegin(); graphTile != tilesToPass.rend(); graphTile++)
			{
				AIPathNodeInfo n;
				auto & node = getNode(*graphTile);

				n.coord = graphTile->coord;
				n.cost = cost;
				n.turns = static_cast<ui8>(cost) + 1; // just in case lets select worst scenario
				n.danger = danger;
				n.targetHero = hero;
				n.parentIndex = -1;
				n.specialAction = node.specialAction;
				
				if(node.linkDanger > 0)
				{
					auto additionalLoss = ai->pathfinder->getStorage()->evaluateArmyLoss(path.targetHero, strength, node.linkDanger);
					loss += additionalLoss;

					if(strength > additionalLoss)
						strength -= additionalLoss;
					else
					{
						strength = 0;
						break;
					}
				}

				if(n.specialAction)
				{
					n.actionIsBlocked = !n.specialAction->canAct(ai, n);
				}

				for(auto & node : path.nodes)
				{
					node.parentIndex++;
				}

				path.nodes.insert(path.nodes.begin(), n);
			}

			if(strength == 0)
			{
				continue;
			}

			path.armyLoss += loss;
			path.targetObjectDanger = ai->dangerEvaluator->evaluateDanger(tile, path.targetHero, !allowBattle);
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

		uint64_t danger = targetNode.linkDanger;
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
			vstd::amax(danger, currentNode.linkDanger);
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
			path.targetObjectDanger = ai->dangerEvaluator->evaluateDanger(tile, path.targetHero, !allowBattle);
			path.targetObjectArmyLoss = ai->pathfinder->getStorage()->evaluateArmyLoss(path.targetHero, path.heroArmy->getArmyStrength(), path.targetObjectDanger);

			AIPathNodeInfo n;

			n.targetHero = hero;
			n.parentIndex = -1;

			// final node
			n.coord = tile;
			n.cost = targetNode.cost;
			n.turns = static_cast<ui8>(targetNode.cost);
			n.danger = danger;
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
				n.danger = danger;
				n.specialAction = node.specialAction;
				n.parentIndex = path.nodes.size();

				if(n.specialAction)
				{
					n.actionIsBlocked = !n.specialAction->canAct(ai, n);
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
