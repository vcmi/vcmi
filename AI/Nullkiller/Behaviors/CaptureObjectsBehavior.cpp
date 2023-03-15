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

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

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

Goals::TGoalVec CaptureObjectsBehavior::getVisitGoals(const std::vector<AIPath> & paths, const CGObjectInstance * objToVisit)
{
	Goals::TGoalVec tasks;

	tasks.reserve(paths.size());

	const AIPath * closestWay = nullptr;
	std::vector<ExecuteHeroChain *> waysToVisitObj;

	for(auto & path : paths)
	{
		tasks.push_back(sptr(Goals::Invalid()));

#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Path found %s", path.toString());
#endif

		if(ai->nullkiller->dangerHitMap->enemyCanKillOurHeroesAlongThePath(path))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Target hero can be killed by enemy. Our power %lld", path.getHeroStrength());
#endif
			continue;
		}

		if(objToVisit && !shouldVisit(ai->nullkiller.get(), path.targetHero, objToVisit))
			continue;

		auto hero = path.targetHero;
		auto danger = path.getTotalDanger();

		if(ai->nullkiller->heroManager->getHeroRole(hero) == HeroRole::SCOUT && path.exchangeCount > 1)
			continue;

		auto firstBlockedAction = path.getFirstBlockedAction();
		if(firstBlockedAction)
		{
			auto subGoal = firstBlockedAction->decompose(path.targetHero);

#if NKAI_TRACE_LEVEL >= 2
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

		auto isSafe = isSafeToVisit(hero, path.heroArmy, danger);

#if NKAI_TRACE_LEVEL >= 2
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

			auto heroRole = ai->nullkiller->heroManager->getHeroRole(path.targetHero);

			if(heroRole == HeroRole::SCOUT
				&& (!closestWay || closestWay->movementCost() > path.movementCost()))
			{
				closestWay = &path;
			}

			if(!ai->nullkiller->arePathHeroesLocked(path))
			{
				waysToVisitObj.push_back(newWay);
				tasks[tasks.size() - 1] = sharedPtr;
			}
		}
	}

	if(closestWay)
	{
		for(auto way : waysToVisitObj)
		{
			way->closestWayRatio
				= closestWay->movementCost() / way->getPath().movementCost();
		}
	}

	return tasks;
}

Goals::TGoalVec CaptureObjectsBehavior::decompose() const
{
	Goals::TGoalVec tasks;

	auto captureObjects = [&](const std::vector<const CGObjectInstance*> & objs) -> void
	{
		if(objs.empty())
		{
			return;
		}

		logAi->debug("Scanning objects, count %d", objs.size());

		for(auto objToVisit : objs)
		{
			if(!objectMatchesFilter(objToVisit))
				continue;
	
#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Checking object %s, %s", objToVisit->getObjectName(), objToVisit->visitablePos().toString());
#endif

			const int3 pos = objToVisit->visitablePos();

			auto paths = ai->nullkiller->pathfinder->getPathInfo(pos);
			std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;
			std::shared_ptr<ExecuteHeroChain> closestWay;
					
#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Found %d paths", paths.size());
#endif
			vstd::concatenate(tasks, getVisitGoals(paths, objToVisit));
		}

		vstd::erase_if(tasks, [](TSubgoal task) -> bool
		{
			return task->invalid();
		});
	};

	if(specificObjects)
	{
		captureObjects(objectsToCapture);
	}
	else if(objectTypes.size())
	{
		captureObjects(
			std::vector<const CGObjectInstance *>(
				ai->nullkiller->memory->visitableObjs.begin(),
				ai->nullkiller->memory->visitableObjs.end()));
	}
	else
	{
		captureObjects(ai->nullkiller->objectClusterizer->getNearbyObjects());

		if(tasks.empty())
			captureObjects(ai->nullkiller->objectClusterizer->getFarObjects());
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
