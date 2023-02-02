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
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "lib/mapping/CMap.h" //for victory conditions

namespace NKAI
{

void ObjectCluster::addObject(const CGObjectInstance * obj, const AIPath & path, float priority)
{
	ClusterObjects::accessor info;

	objects.insert(info, ClusterObjects::value_type(obj, ClusterObjectInfo()));

	if(info->second.priority < priority)
	{
		info->second.priority = priority;
		info->second.movementCost = path.movementCost() - path.firstNode().cost;
		info->second.danger = path.targetObjectDanger;
		info->second.turn = path.turn();
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

	auto closestPair = *vstd::minElementByFun(objects, [&](const std::pair<const CGObjectInstance *, ClusterObjectInfo> & pair) -> int
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

		if(isObjectPassable(ai, blocker))
			continue;

		if(blocker->ID == Obj::GARRISON
			|| blocker->ID == Obj::GARRISON2)
		{
			if(dynamic_cast<const CArmedInstance *>(blocker)->getArmyStrength() == 0)
				continue;
			else
				return blocker;
		}

		if(blocker->ID == Obj::MONSTER
			|| blocker->ID == Obj::BORDERGUARD
			|| blocker->ID == Obj::BORDER_GATE
			|| blocker->ID == Obj::SHIPYARD
			|| (blocker->ID == Obj::QUEST_GUARD && node->actionIsBlocked))
		{
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

	if((obj->ID != Obj::CREATURE_GENERATOR1 && vstd::contains(ai->memory->alreadyVisited, obj))
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
	auto start = std::chrono::high_resolution_clock::now();

	nearObjects.reset();
	farObjects.reset();
	blockedObjects.clear();

	Obj ignoreObjects[] = {
		Obj::BOAT,
		Obj::EYE_OF_MAGI,
		Obj::MONOLITH_ONE_WAY_ENTRANCE,
		Obj::MONOLITH_ONE_WAY_EXIT,
		Obj::MONOLITH_TWO_WAY,
		Obj::SUBTERRANEAN_GATE,
		Obj::WHIRLPOOL,
		Obj::BUOY,
		Obj::SIGN,
		Obj::GARRISON,
		Obj::MONSTER,
		Obj::GARRISON2,
		Obj::BORDERGUARD,
		Obj::QUEST_GUARD,
		Obj::BORDER_GATE,
		Obj::REDWOOD_OBSERVATORY,
		Obj::CARTOGRAPHER,
		Obj::PILLAR_OF_FIRE
	};

	logAi->debug("Begin object clusterization");

	std::vector<const CGObjectInstance *> objs(
		ai->memory->visitableObjs.begin(),
		ai->memory->visitableObjs.end());

	parallel_for(blocked_range<size_t>(0, objs.size()), [&](const blocked_range<size_t> & r)
	{
		auto priorityEvaluator = ai->priorityEvaluators->acquire();

		for(int i = r.begin(); i != r.end(); i++)
		{
			auto obj = objs[i];

			if(!shouldVisitObject(obj))
				return;

#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Check object %s%s.", obj->getObjectName(), obj->visitablePos().toString());
#endif

			auto paths = ai->pathfinder->getPathInfo(obj->visitablePos());

			if(paths.empty())
			{
#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("No paths found.");
#endif
				continue;
			}

			std::sort(paths.begin(), paths.end(), [](const AIPath & p1, const AIPath & p2) -> bool
			{
				return p1.movementCost() < p2.movementCost();
			});

			if(vstd::contains(ignoreObjects, obj->ID))
			{
				farObjects.addObject(obj, paths.front(), 0);

#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Object ignored. Moved to far objects with path %s", paths.front().toString());
#endif

				continue;
			}

			std::set<const CGHeroInstance *> heroesProcessed;

			for(auto & path : paths)
			{
#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Checking path %s", path.toString());
#endif

				if(!shouldVisit(ai, path.targetHero, obj))
				{
#if NKAI_TRACE_LEVEL >= 2
					logAi->trace("Hero %s does not need to visit %s", path.targetHero->name, obj->getObjectName());
#endif
					continue;
				}

				if(path.nodes.size() > 1)
				{
					auto blocker = getBlocker(path);

					if(blocker)
					{
						if(vstd::contains(heroesProcessed, path.targetHero))
						{
#if NKAI_TRACE_LEVEL >= 2
							logAi->trace("Hero %s is already processed.", path.targetHero->name);
#endif
							continue;
						}

						heroesProcessed.insert(path.targetHero);

						float priority = priorityEvaluator->evaluate(Goals::sptr(Goals::ExecuteHeroChain(path, obj)));

						if(priority < MIN_PRIORITY)
							continue;

						ClusterMap::accessor cluster;
						blockedObjects.insert(
							cluster,
							ClusterMap::value_type(blocker, std::make_shared<ObjectCluster>(blocker)));

						cluster->second->addObject(obj, path, priority);

#if NKAI_TRACE_LEVEL >= 2
						logAi->trace("Path added to cluster %s%s", blocker->getObjectName(), blocker->visitablePos().toString());
#endif
						continue;
					}
				}

				heroesProcessed.insert(path.targetHero);

				float priority = priorityEvaluator->evaluate(Goals::sptr(Goals::ExecuteHeroChain(path, obj)));

				if(priority < MIN_PRIORITY)
					continue;

				bool interestingObject = path.turn() <= 2 || priority > 0.5f;

				if(interestingObject)
				{
					nearObjects.addObject(obj, path, priority);
				}
				else
				{
					farObjects.addObject(obj, path, priority);
				}

#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Path %s added to %s objects. Turn: %d, priority: %f",
					path.toString(),
					interestingObject ? "near" : "far",
					path.turn(),
					priority);
#endif
			}
		}
	});

	logAi->trace("Near objects count: %i", nearObjects.objects.size());
	logAi->trace("Far objects count: %i", farObjects.objects.size());

	for(auto pair : blockedObjects)
	{
		logAi->trace("Cluster %s %s count: %i", pair.first->getObjectName(), pair.first->visitablePos().toString(), pair.second->objects.size());

#if NKAI_TRACE_LEVEL >= 1
		for(auto obj : pair.second->getObjects())
		{
			logAi->trace("Object %s %s", obj->getObjectName(), obj->visitablePos().toString());
		}
#endif
	}

	logAi->trace("Clusterization complete in %ld", timeElapsed(start));
}

}
