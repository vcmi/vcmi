/*
* CaptureObjectsBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/Composition.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/Invalid.h"
#include "CaptureObjectsBehavior.h"
#include "../AIUtility.h"

namespace NK2AI
{

using namespace Goals;

template <typename T>
bool vectorEquals(const std::vector<T> & v1, const std::vector<T> & v2)
{
	return vstd::contains_if(v1, [&](T o) -> bool
	{
		return vstd::contains(v2, o);
	});
}

std::string CaptureObjectsBehavior::toString() const
{
	return "Capture objects";
}

bool CaptureObjectsBehavior::operator==(const CaptureObjectsBehavior & other) const
{
	if(specificObjects != other.specificObjects)
		return false;

	if(specificObjects)
		return vectorEquals(objectsToCapture, other.objectsToCapture);

	return vectorEquals(objectTypes, other.objectTypes)
		&& vectorEquals(objectSubTypes, other.objectSubTypes);
}

Goals::TGoalVec CaptureObjectsBehavior::getVisitGoals(
	const std::vector<AIPath> & paths,
	const Nullkiller * nullkiller,
	const CGObjectInstance * objToVisit,
	bool force)
{
	Goals::TGoalVec tasks;

	tasks.reserve(paths.size());

	std::unordered_map<HeroRole, const AIPath *> closestWaysByRole;
	std::vector<ExecuteHeroChain *> waysToVisitObj;

	for(const auto & path : paths)
	{
		tasks.push_back(sptr(Goals::Invalid()));

#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace("Path found %s", path.toString());
#endif

		if(objToVisit && !force && !shouldVisit(nullkiller, path.targetHero, objToVisit))
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Hero %s should not visit obj %s", path.targetHero->getNameTranslated(), objToVisit->getObjectName());
#endif
			continue;
		}

		const auto * hero = path.targetHero;
		const auto danger = path.getTotalDanger();
		if (hero->getOwner() != nullkiller->playerID)
			continue;

		if(nullkiller->heroManager->getHeroRoleOrDefaultInefficient(hero) == HeroRole::SCOUT
			&& (path.getTotalDanger() == 0 || path.turn() > 0)
			&& path.exchangeCount > 1)
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Hero %s is SCOUT, chain used and no danger", path.targetHero->getNameTranslated());
#endif
			continue;
		}

		auto firstBlockedAction = path.getFirstBlockedAction();
		if(firstBlockedAction)
		{
			auto subGoal = firstBlockedAction->decompose(nullkiller, path.targetHero);

#if NK2AI_TRACE_LEVEL >= 2
			// subGoal->toString() was crashing sometims before thread local fix with SET_GLOBAL_STATE_TBB
			logAi->trace("Decomposing special action %s returns %s", firstBlockedAction->toString(), subGoal->toString());
#endif

			if(!subGoal->invalid())
			{
				Composition composition;

				composition.addNext(ExecuteHeroChain(path, objToVisit));
				composition.addNext(subGoal);

				tasks[tasks.size() - 1] = sptr(composition);
			}

			continue;
		}

		auto isSafe = isSafeToVisit(hero, path.heroArmy, danger, nullkiller->settings->getSafeAttackRatio());

#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace(
			"It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld",
			isSafe ? "safe" : "not safe",
			objToVisit ? objToVisit->getObjectName() : path.targetTile().toString(),
			hero->getObjectName(),
			path.getHeroStrength(),
			danger,
			path.getTotalArmyLoss());
#endif

		if(isSafe)
		{
			auto newWay = new ExecuteHeroChain(path, objToVisit);
			TSubgoal sharedPtr;
			sharedPtr.reset(newWay);
			auto heroRole = nullkiller->heroManager->getHeroRoleOrDefaultInefficient(path.targetHero);

			auto & closestWay = closestWaysByRole[heroRole];
			if(!closestWay || closestWay->movementCost() > path.movementCost())
			{
				closestWay = &path;
			}

			if(!nullkiller->arePathHeroesLocked(path))
			{
				waysToVisitObj.push_back(newWay);
				tasks[tasks.size() - 1] = sharedPtr;
			}
		}
	}

	for(auto * way : waysToVisitObj)
	{
		auto heroRole = nullkiller->heroManager->getHeroRoleOrDefaultInefficient(way->getPath().targetHero);
		const auto * closestWay = closestWaysByRole[heroRole];

		if(closestWay)
		{
			way->closestWayRatio = closestWay->movementCost() / way->getPath().movementCost();
		}
	}

	return tasks;
}

void CaptureObjectsBehavior::decomposeObjects(
	Goals::TGoalVec & result,
	const std::vector<const CGObjectInstance *> & objs,
	const Nullkiller * nullkiller) const
{
	if(objs.empty())
		return;

	std::mutex sync;
	logAi->debug("Scanning objects, count %d", objs.size());
	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, objs.size()),
		[this, &objs, &sync, &result, nullkiller](const tbb::blocked_range<size_t> & r)
		{
			std::vector<AIPath> paths;
			Goals::TGoalVec tasksLocal;

			for(auto i = r.begin(); i != r.end(); i++)
			{
				const auto * objToVisit = objs[i];
				if(!objectMatchesFilter(objToVisit))
					continue;

#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Checking object %s, %s", objToVisit->getObjectName(), objToVisit->visitablePos().toString());
#endif

				// FIXME: Mircea: This one uses the deleted hero
				nullkiller->pathfinder->calculatePathInfo(paths, objToVisit->visitablePos(), nullkiller->isObjectGraphAllowed());

#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Found %d paths", paths.size());
#endif
				vstd::concatenate(tasksLocal, getVisitGoals(paths, nullkiller, objToVisit, specificObjects));
			}

			std::lock_guard lock(sync); // FIXME: consider using tbb::parallel_reduce instead to avoid mutex overhead
			vstd::concatenate(result, tasksLocal);
		}
	);
}

Goals::TGoalVec CaptureObjectsBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;

	vstd::erase_if(tasks, [](TSubgoal task) -> bool
	{
		return task->invalid();
	});

	if(specificObjects)
	{
		decomposeObjects(tasks, objectsToCapture, aiNk);
	}
	else if(objectTypes.size())
	{
		decomposeObjects(tasks, aiNk->memory->visitableIdsToObjsVector(*aiNk->cc), aiNk);
	}
	else
	{
		decomposeObjects(tasks, aiNk->objectClusterizer->getNearbyObjects(), aiNk);

		if(tasks.empty() || aiNk->getScanDepth() != ScanDepth::SMALL)
			decomposeObjects(tasks, aiNk->objectClusterizer->getFarObjects(), aiNk);
	}

	return tasks;
}

bool CaptureObjectsBehavior::objectMatchesFilter(const CGObjectInstance * obj) const
{
	if(objectTypes.size() && !vstd::contains(objectTypes, obj->ID.num))
	{
		return false;
	}

	if(objectSubTypes.size() && !vstd::contains(objectSubTypes, obj->subID))
	{
		return false;
	}

	return true;
}

}
