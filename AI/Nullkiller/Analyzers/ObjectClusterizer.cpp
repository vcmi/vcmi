/*
* DangerHitMapAnalyzer.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ObjectClusterizer.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../VCAI.h"
#include "../Engine/Nullkiller.h"
#include "lib/mapping/CMap.h" //for victory conditions

void ObjectCluster::addObject(const CGObjectInstance * obj, const AIPath & path, float priority)
{
	auto & info = objects[obj];

	if(info.priority < priority)
	{
		info.priority = priority;
		info.movementCost = path.movementCost() - path.firstNode().cost;
		info.danger = path.targetObjectDanger;
		info.turn = path.turn();
	}
}

const CGObjectInstance * ObjectCluster::calculateCenter() const
{
	auto v = getObjects();
	auto tile = int3(0);
	float priority = 0;

	for(auto pair : objects)
	{
		auto newPoint = pair.first->visitablePos();
		float newPriority = std::pow(pair.second.priority, 4); // lets make high priority targets more weghtful
		int3 direction = newPoint - tile;
		float priorityRatio = newPriority / (priority + newPriority);

		tile += direction * priorityRatio;
		priority += newPriority;
	}

	auto closestPair = *vstd::minElementByFun(objects, [&](const std::pair<const CGObjectInstance *, ObjectInfo> & pair) -> int
	{
		return pair.first->visitablePos().dist2dSQ(tile);
	});

	return closestPair.first;
}

std::vector<const CGObjectInstance *> ObjectCluster::getObjects() const
{
	std::vector<const CGObjectInstance *> result;

	for(auto pair : objects)
	{
		result.push_back(pair.first);
	}

	return result;
}

std::vector<const CGObjectInstance *> ObjectClusterizer::getNearbyObjects() const
{
	return nearObjects.getObjects();
}

std::vector<const CGObjectInstance *> ObjectClusterizer::getFarObjects() const
{
	return farObjects.getObjects();
}

std::vector<std::shared_ptr<ObjectCluster>> ObjectClusterizer::getLockedClusters() const
{
	std::vector<std::shared_ptr<ObjectCluster>> result;

	for(auto pair : blockedObjects)
	{
		result.push_back(pair.second);
	}

	return result;
}

const CGObjectInstance * ObjectClusterizer::getBlocker(const AIPath & path) const
{
	for(auto node = path.nodes.rbegin(); node != path.nodes.rend(); node++)
	{
		auto guardPos = ai->cb->getGuardingCreaturePosition(node->coord);
		auto blockers = ai->cb->getVisitableObjs(node->coord);
		
		if(guardPos.valid())
		{
			auto guard = ai->cb->getTopObj(ai->cb->getGuardingCreaturePosition(node->coord));

			if(guard)
			{
				blockers.insert(blockers.begin(), guard);
			}
		}

		if(node->specialAction && node->actionIsBlocked)
		{
			auto blockerObject = node->specialAction->targetObject();

			if(blockerObject)
			{
				blockers.push_back(blockerObject);
			}
		}

		if(blockers.empty())
			continue;

		auto blocker = blockers.front();

		if(blocker->ID == Obj::GARRISON
			|| blocker->ID == Obj::MONSTER
			|| blocker->ID == Obj::GARRISON2
			|| blocker->ID == Obj::BORDERGUARD
			|| blocker->ID == Obj::QUEST_GUARD
			|| blocker->ID == Obj::BORDER_GATE
			|| blocker->ID == Obj::SHIPYARD)
		{
			if(!isObjectPassable(blocker))
				return blocker;
		}
	}

	return nullptr;
}

bool ObjectClusterizer::shouldVisitObject(const CGObjectInstance * obj) const
{
	if(isObjectRemovable(obj))
	{
		return true;
	}

	const int3 pos = obj->visitablePos();

	if(obj->ID != Obj::CREATURE_GENERATOR1 && vstd::contains(ai->memory->alreadyVisited, obj)
		|| obj->wasVisited(ai->playerID))
	{
		return false;
	}

	auto playerRelations = ai->cb->getPlayerRelations(ai->playerID, obj->tempOwner);

	if(playerRelations != PlayerRelations::ENEMIES && !isWeeklyRevisitable(obj))
	{
		return false;
	}

	//it may be hero visiting this obj
	//we don't try visiting object on which allied or owned hero stands
	// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
	const CGObjectInstance * topObj = ai->cb->getTopObj(pos);

	if(!topObj)
		return false; // partly visible obj but its visitable pos is not visible.

	if(topObj->ID == Obj::HERO && ai->cb->getPlayerRelations(ai->playerID, topObj->tempOwner) != PlayerRelations::ENEMIES)
		return false;
	else
		return true; //all of the following is met
}

void ObjectClusterizer::clusterize()
{
	nearObjects.reset();
	farObjects.reset();
	blockedObjects.clear();

	Obj ignoreObjects[] = {
		Obj::MONSTER,
		Obj::SIGN,
		Obj::REDWOOD_OBSERVATORY,
		Obj::MONOLITH_TWO_WAY,
		Obj::MONOLITH_ONE_WAY_ENTRANCE,
		Obj::MONOLITH_ONE_WAY_EXIT,
		Obj::BUOY
	};

	logAi->debug("Begin object clusterization");

	for(const CGObjectInstance * obj : ai->memory->visitableObjs)
	{
		if(!shouldVisitObject(obj))
			continue;

		auto paths = ai->pathfinder->getPathInfo(obj->visitablePos());

		if(paths.empty())
			continue;

		std::sort(paths.begin(), paths.end(), [](const AIPath & p1, const AIPath & p2) -> bool
		{
			return p1.movementCost() < p2.movementCost();
		});

		if(vstd::contains(ignoreObjects, obj->ID))
		{
			farObjects.addObject(obj, paths.front(), 0);

			continue;
		}
		
		bool added = false;
		bool directlyAccessible = false;
		std::set<const CGHeroInstance *> heroesProcessed;

		for(auto & path : paths)
		{
			if(vstd::contains(heroesProcessed, path.targetHero))
				continue;

			heroesProcessed.insert(path.targetHero);

			if(path.nodes.size() > 1)
			{
				auto blocker = getBlocker(path);

				if(blocker)
				{
					auto cluster = blockedObjects[blocker];

					if(!cluster)
					{
						cluster.reset(new ObjectCluster(blocker));
						blockedObjects[blocker] = cluster;
					}

					if(!vstd::contains(cluster->objects, obj))
					{
						float priority = ai->priorityEvaluator->evaluate(Goals::sptr(Goals::ExecuteHeroChain(path, obj)));

						cluster->addObject(obj, path, priority);

						added = true;
					}
				}
			}
			else
			{
				directlyAccessible = true;
			}
		}

		if(!added || directlyAccessible)
		{
			AIPath & shortestPath = paths.front();
			float priority = ai->priorityEvaluator->evaluate(Goals::sptr(Goals::ExecuteHeroChain(shortestPath, obj)));

			if(shortestPath.turn() <= 2 || priority > 0.6f)
				nearObjects.addObject(obj, shortestPath, 0);
			else
				farObjects.addObject(obj, shortestPath, 0);
		}
	}

	logAi->trace("Near objects count: %i", nearObjects.objects.size());
	logAi->trace("Far objects count: %i", farObjects.objects.size());
	for(auto pair : blockedObjects)
	{
		logAi->trace("Cluster %s %s count: %i", pair.first->getObjectName(), pair.first->visitablePos().toString(), pair.second->objects.size());

#if AI_TRACE_LEVEL >= 1
		for(auto obj : pair.second->getObjects())
		{
			logAi->trace("Object %s %s", obj->getObjectName(), obj->visitablePos().toString());
		}
#endif
	}
}