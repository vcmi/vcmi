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

namespace NKAI
{

void ObjectCluster::addObject(const CGObjectInstance * obj, const AIPath & path, float priority)
{
	ClusterObjects::accessor info;

	objects.insert(info, ClusterObjects::value_type(obj->id, ClusterObjectInfo()));

	if(info->second.priority < priority)
	{
		info->second.priority = priority;
		info->second.movementCost = path.movementCost() - path.firstNode().cost;
		info->second.danger = path.targetObjectDanger;
		info->second.turn = path.turn();
	}
}

const CGObjectInstance * ObjectCluster::calculateCenter(const CPlayerSpecificInfoCallback * cb) const
{
	auto tile = int3(0);
	float priority = 0;

	for(auto & pair : objects)
	{
		auto newPoint = cb->getObj(pair.first)->visitablePos();
		float newPriority = std::pow(pair.second.priority, 4); // lets make high priority targets more weghtful
		int3 direction = newPoint - tile;
		float priorityRatio = newPriority / (priority + newPriority);

		tile += direction * priorityRatio;
		priority += newPriority;
	}

	auto closestPair = *vstd::minElementByFun(objects, [&](const std::pair<ObjectInstanceID, ClusterObjectInfo> & pair) -> int
	{
		return cb->getObj(pair.first)->visitablePos().dist2dSQ(tile);
	});

	return cb->getObj(closestPair.first);
}

std::vector<const CGObjectInstance *> ObjectCluster::getObjects(const CPlayerSpecificInfoCallback * cb) const
{
	std::vector<const CGObjectInstance *> result;

	for(auto & pair : objects)
	{
		result.push_back(cb->getObj(pair.first));
	}

	return result;
}

std::vector<const CGObjectInstance *> ObjectClusterizer::getNearbyObjects() const
{
	return nearObjects.getObjects(ai->cb.get());
}

std::vector<const CGObjectInstance *> ObjectClusterizer::getFarObjects() const
{
	return farObjects.getObjects(ai->cb.get());
}

std::vector<std::shared_ptr<ObjectCluster>> ObjectClusterizer::getLockedClusters() const
{
	std::vector<std::shared_ptr<ObjectCluster>> result;

	for(auto & pair : blockedObjects)
	{
		result.push_back(pair.second);
	}

	return result;
}

std::optional<const CGObjectInstance *> ObjectClusterizer::getBlocker(const AIPathNodeInfo & node) const
{
	std::vector<const CGObjectInstance *> blockers = {};

	if(node.layer == EPathfindingLayer::LAND || node.layer == EPathfindingLayer::SAIL)
	{
		auto guardPos = ai->cb->getGuardingCreaturePosition(node.coord);

		if (ai->cb->isVisible(node.coord))
			blockers = ai->cb->getVisitableObjs(node.coord);

		if(guardPos.isValid() && ai->cb->isVisible(guardPos))
		{
			auto guard = ai->cb->getTopObj(ai->cb->getGuardingCreaturePosition(node.coord));

			if(guard)
			{
				blockers.insert(blockers.begin(), guard);
			}
		}
	}

	if(node.specialAction && node.actionIsBlocked)
	{
		auto blockerObject = node.specialAction->targetObject();

		if(blockerObject)
		{
			blockers.insert(blockers.begin(), blockerObject);
		}
	}

	if(blockers.empty())
		return std::optional< const CGObjectInstance *>();

	auto blocker = blockers.front();

	if(isObjectPassable(ai, blocker))
		return std::optional< const CGObjectInstance *>();

	if(blocker->ID == Obj::GARRISON
		|| blocker->ID == Obj::GARRISON2)
	{
		if(dynamic_cast<const CArmedInstance *>(blocker)->getArmyStrength() == 0)
			return std::optional< const CGObjectInstance *>();
		else
			return blocker;
	}

	if(blocker->ID == Obj::MONSTER
		|| blocker->ID == Obj::BORDERGUARD
		|| blocker->ID == Obj::BORDER_GATE
		|| blocker->ID == Obj::SHIPYARD
		|| (blocker->ID == Obj::QUEST_GUARD && node.actionIsBlocked))
	{
		return blocker;
	}

	auto danger = ai->dangerEvaluator->evaluateDanger(blocker);

	if(danger > 0 && blocker->isBlockedVisitable() && isObjectRemovable(blocker))
	{
		return blocker;
	}

	return std::optional< const CGObjectInstance *>();
}

const CGObjectInstance * ObjectClusterizer::getBlocker(const AIPath & path) const
{
	for(auto node = path.nodes.rbegin(); node != path.nodes.rend(); node++)
	{
		auto blocker = getBlocker(*node);

		if(blocker)
			return *blocker;
	}

	return nullptr;
}

void ObjectClusterizer::invalidate(ObjectInstanceID id)
{
	nearObjects.objects.erase(id);
	farObjects.objects.erase(id);
	invalidated.push_back(id);

	for(auto & c : blockedObjects)
	{
		c.second->objects.erase(id);
	}
}

void ObjectClusterizer::validateObjects()
{
	std::vector<ObjectInstanceID> toRemove;

	auto scanRemovedObjects = [this, &toRemove](const ObjectCluster & cluster)
	{
		for(auto & pair : cluster.objects)
		{
			if(!ai->cb->getObj(pair.first, false))
				toRemove.push_back(pair.first);
		}
	};

	scanRemovedObjects(nearObjects);
	scanRemovedObjects(farObjects);

	for(auto & pair : blockedObjects)
	{
		if(!ai->cb->getObj(pair.first, false) || pair.second->objects.empty())
			toRemove.push_back(pair.first);
		else
			scanRemovedObjects(*pair.second);
	}

	vstd::removeDuplicates(toRemove);

	for(auto id : toRemove)
	{
		onObjectRemoved(id);
	}
}

void ObjectClusterizer::onObjectRemoved(ObjectInstanceID id)
{
	invalidate(id);

	vstd::erase_if_present(invalidated, id);

	NKAI::ClusterMap::accessor cluster;
	
	if(blockedObjects.find(cluster, id))
	{
		for(auto & unlocked : cluster->second->objects)
		{
			invalidated.push_back(unlocked.first);
		}

		blockedObjects.erase(cluster);
	}
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

	if(playerRelations != PlayerRelations::ENEMIES && !isWeeklyRevisitable(ai, obj))
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

Obj ObjectClusterizer::IgnoredObjectTypes[] = {
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

void ObjectClusterizer::clusterize()
{
	if(isUpToDate)
	{
		validateObjects();
	}

	if(isUpToDate && invalidated.empty())
		return;
		
	auto start = std::chrono::high_resolution_clock::now();

	logAi->debug("Begin object clusterization");

	std::vector<const CGObjectInstance *> objs;
	
	if(isUpToDate)
	{
		for(auto id : invalidated)
		{
			auto obj = cb->getObj(id, false);

			if(obj)
			{
				objs.push_back(obj);
			}
		}

		invalidated.clear();
	}
	else
	{
		nearObjects.reset();
		farObjects.reset();
		blockedObjects.clear();
		invalidated.clear();

		objs = std::vector<const CGObjectInstance *>(
			ai->memory->visitableObjs.begin(),
			ai->memory->visitableObjs.end());
	}

#if NKAI_TRACE_LEVEL == 0
	tbb::parallel_for(tbb::blocked_range<size_t>(0, objs.size()), [&](const tbb::blocked_range<size_t> & r) {
#else
	tbb::blocked_range<size_t> r(0, objs.size());
#endif
		auto priorityEvaluator = ai->priorityEvaluators->acquire();
		auto heroes = ai->cb->getHeroesInfo();
		std::vector<AIPath> pathCache;

		for(int i = r.begin(); i != r.end(); i++)
		{
			clusterizeObject(objs[i], priorityEvaluator.get(), pathCache, heroes);
		}
#if NKAI_TRACE_LEVEL == 0
	});
#endif

	logAi->trace("Near objects count: %i", nearObjects.objects.size());
	logAi->trace("Far objects count: %i", farObjects.objects.size());

	for(auto pair : blockedObjects)
	{
		auto blocker = cb->getObj(pair.first);

		logAi->trace("Cluster %s %s count: %i", blocker->getObjectName(), blocker->visitablePos().toString(), pair.second->objects.size());

#if NKAI_TRACE_LEVEL >= 1
		for(auto obj : pair.second->getObjects(ai->cb.get()))
		{
			logAi->trace("Object %s %s", obj->getObjectName(), obj->visitablePos().toString());
		}
#endif
	}

	isUpToDate = true;

	logAi->trace("Clusterization complete in %ld", timeElapsed(start));
}

void ObjectClusterizer::clusterizeObject(
	const CGObjectInstance * obj,
	PriorityEvaluator * priorityEvaluator,
	std::vector<AIPath> & pathCache,
	std::vector<const CGHeroInstance *> & heroes)
{
	if(!shouldVisitObject(obj))
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Skip object %s%s.", obj->getObjectName(), obj->visitablePos().toString());
#endif
		return;
	}

#if NKAI_TRACE_LEVEL >= 2
	logAi->trace("Check object %s%s.", obj->getObjectName(), obj->visitablePos().toString());
#endif

	if(ai->isObjectGraphAllowed())
	{
		ai->pathfinder->calculateQuickPathsWithBlocker(pathCache, heroes, obj->visitablePos());
	}
	else
		ai->pathfinder->calculatePathInfo(pathCache, obj->visitablePos(), false);

	if(pathCache.empty())
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("No paths found.");
#endif
		return;
	}

	std::sort(pathCache.begin(), pathCache.end(), [](const AIPath & p1, const AIPath & p2) -> bool
		{
			return p1.movementCost() < p2.movementCost();
		});

	if(vstd::contains(IgnoredObjectTypes, obj->ID))
	{
		farObjects.addObject(obj, pathCache.front(), 0);

#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Object ignored. Moved to far objects with path %s", pathCache.front().toString());
#endif

		return;
	}

	std::set<const CGHeroInstance *> heroesProcessed;

	for(auto & path : pathCache)
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("ObjectClusterizer Checking path %s", path.toString());
#endif

		if(ai->heroManager->getHeroRole(path.targetHero) == HeroRole::SCOUT)
		{
			if(path.movementCost() > 2.0f)
			{
#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Path is too far %f", path.movementCost());
#endif
				continue;
			}
		}
		else if(path.movementCost() > 4.0f && obj->ID != Obj::TOWN)
		{
			auto strategicalValue = valueEvaluator.getStrategicalValue(obj);

			if(strategicalValue < 0.3f)
			{
#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Object value is too low %f", strategicalValue);
#endif
				continue;
			}
		}

		if(!shouldVisit(ai, path.targetHero, obj))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Hero %s does not need to visit %s", path.targetHero->getObjectName(), obj->getObjectName());
#endif
			continue;
		}

		float priority = 0;

		if(path.nodes.size() > 1)
		{
			auto blocker = getBlocker(path);

			if(blocker)
			{
				if(vstd::contains(heroesProcessed, path.targetHero))
				{
#if NKAI_TRACE_LEVEL >= 2
					logAi->trace("Hero %s is already processed.", path.targetHero->getObjectName());
#endif
					continue;
				}

				heroesProcessed.insert(path.targetHero);

				for (int prio = PriorityEvaluator::PriorityTier::BUILDINGS; prio <= PriorityEvaluator::PriorityTier::MAX_PRIORITY_TIER; ++prio)
				{
					priority = std::max(priority, priorityEvaluator->evaluate(Goals::sptr(Goals::ExecuteHeroChain(path, obj)), prio));
				}

				if(ai->settings->isUseFuzzy() && priority < MIN_PRIORITY)
					continue;
				else if (priority <= 0)
					continue;

				ClusterMap::accessor cluster;
				blockedObjects.insert(
					cluster,
					ClusterMap::value_type(blocker->id, std::make_shared<ObjectCluster>(blocker)));

				cluster->second->addObject(obj, path, priority);

#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Path added to cluster %s%s", blocker->getObjectName(), blocker->visitablePos().toString());
#endif
				continue;
			}
		}

		heroesProcessed.insert(path.targetHero);

		for (int prio = PriorityEvaluator::PriorityTier::BUILDINGS; prio <= PriorityEvaluator::PriorityTier::MAX_PRIORITY_TIER; ++prio)
		{
			priority = std::max(priority, priorityEvaluator->evaluate(Goals::sptr(Goals::ExecuteHeroChain(path, obj)), prio));
		}

		if (ai->settings->isUseFuzzy() && priority < MIN_PRIORITY)
			continue;
		else if (priority <= 0)
			continue;

		bool interestingObject = path.turn() <= 2 || priority > (ai->settings->isUseFuzzy() ? 0.5f : 0);

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

}
