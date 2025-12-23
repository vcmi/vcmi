/*
* ObjectGraphCalculator.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ObjectGraphCalculator.h"
#include "AIPathfinderConfig.h"
#include "../../../lib/callback/GameRandomizer.h"
#include "../../../lib/mapping/CMap.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/logging/VisualLogger.h"
#include "Actions/QuestAction.h"
#include "../pforeach.h"

namespace NKAI
{

ObjectGraphCalculator::ObjectGraphCalculator(ObjectGraph * target, const Nullkiller * ai)
	:ai(ai), target(target), syncLock()
{
}

void ObjectGraphCalculator::setGraphObjects()
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

void ObjectGraphCalculator::calculateConnections()
{
	updatePaths();

	std::vector<AIPath> pathCache;

	foreach_tile_pos(ai->cb.get(), [this, &pathCache](const CPlayerSpecificInfoCallback * cb, const int3 & pos)
		{
			calculateConnections(pos, pathCache);
		});

	removeExtraConnections();
}

float ObjectGraphCalculator::getNeighborConnectionsCost(const int3 & pos, std::vector<AIPath> & pathCache)
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

void ObjectGraphCalculator::addMinimalDistanceJunctions()
{
	tbb::concurrent_unordered_set<int3, std::hash<int3>> junctions;

	pforeachTilePaths(ai->cb->getMapSize(), ai, [this, &junctions](const int3 & pos, std::vector<AIPath> & paths)
		{
			if(target->hasNodeAt(pos))
				return;

			if(ai->cb->getGuardingCreaturePosition(pos).isValid())
				return;

			ConnectionCostInfo currentCost = getConnectionsCost(paths);

			if(currentCost.connectionsCount <= 2)
				return;

			float neighborCost = getNeighborConnectionsCost(pos, paths);

			if(currentCost.avg < neighborCost)
			{
				junctions.insert(pos);
			}
		});

	for(auto pos : junctions)
	{
		addJunctionActor(pos);
	}
}

void ObjectGraphCalculator::updatePaths()
{
	PathfinderSettings ps;

	ps.mainTurnDistanceLimit = 5;
	ps.scoutTurnDistanceLimit = 1;
	ps.allowBypassObjects = false;

	ai->pathfinder->updatePaths(actors, ps);
}

void ObjectGraphCalculator::calculateConnections(const int3 & pos, std::vector<AIPath> & pathCache)
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

				auto danger = ai->dangerEvaluator->evaluateDanger(pos, path.targetHero, true);
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

			if(guardPos.isValid() && guardPos != pos1 && guardPos != pos2)
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

			auto danger = ai->dangerEvaluator->evaluateDanger(pos2, path1.targetHero, true);

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

bool ObjectGraphCalculator::isExtraConnection(float direct, float side1, float side2) const
{
	float sideRatio = (side1 + side2) / direct;

	return sideRatio < 1.25f && direct > side1 && direct > side2;
}

void ObjectGraphCalculator::removeExtraConnections()
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

void ObjectGraphCalculator::addObjectActor(const CGObjectInstance * obj)
{
	auto objectActor = temporaryActorHeroes.emplace_back(std::make_unique<CGHeroInstance>(obj->cb)).get();

	GameRandomizer randomizer(*obj->cb);
	auto visitablePos = obj->visitablePos();

	objectActor->setOwner(ai->playerID); // lets avoid having multiple colors
	objectActor->initHero(randomizer, static_cast<HeroTypeID>(0));
	objectActor->pos = objectActor->convertFromVisitablePos(visitablePos);
	objectActor->initObj(randomizer);

	if(cb->getTile(visitablePos)->isWater())
	{
		objectActor->setBoat(temporaryBoats.emplace_back(std::make_unique<CGBoat>(objectActor->cb)).get());
	}

	assert(objectActor->visitablePos() == visitablePos);

	actorObjectMap[objectActor] = obj;
	actors[objectActor] = obj->ID == Obj::TOWN || obj->ID == Obj::BOAT ? HeroRole::MAIN : HeroRole::SCOUT;

	target->addObject(obj);

	auto shipyard = dynamic_cast<const IShipyard *>(obj);
		
	if(shipyard && shipyard->bestLocation().isValid())
	{
		int3 virtualBoat = shipyard->bestLocation();
			
		addJunctionActor(virtualBoat, true);
		target->addVirtualBoat(virtualBoat, obj);
	}
}

void ObjectGraphCalculator::addJunctionActor(const int3 & visitablePos, bool isVirtualBoat)
{
	std::lock_guard lock(syncLock);

	auto internalCb = temporaryActorHeroes.front()->cb;
	auto objectActor = temporaryActorHeroes.emplace_back(std::make_unique<CGHeroInstance>(internalCb)).get();

	GameRandomizer randomizer(*internalCb);

	objectActor->setOwner(ai->playerID); // lets avoid having multiple colors
	objectActor->initHero(randomizer, static_cast<HeroTypeID>(0));
	objectActor->pos = objectActor->convertFromVisitablePos(visitablePos);
	objectActor->initObj(randomizer);

	if(isVirtualBoat || ai->cb->getTile(visitablePos)->isWater())
	{
		objectActor->setBoat(temporaryBoats.emplace_back(std::make_unique<CGBoat>(objectActor->cb)).get());
	}

	assert(objectActor->visitablePos() == visitablePos);

	actorObjectMap[objectActor] = nullptr;
	actors[objectActor] = isVirtualBoat ? HeroRole::MAIN : HeroRole::SCOUT;

	target->registerJunction(visitablePos);
}

ConnectionCostInfo ObjectGraphCalculator::getConnectionsCost(std::vector<AIPath> & paths) const
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

}
