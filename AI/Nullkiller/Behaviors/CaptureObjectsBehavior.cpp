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
#include "../VCAI.h"
#include "../Engine/Nullkiller.h"
#include "../AIhelper.h"
#include "../Goals/Composition.h"
#include "../Goals/ExecuteHeroChain.h"
#include "CaptureObjectsBehavior.h"
#include "../AIUtility.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

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

Goals::TGoalVec CaptureObjectsBehavior::decompose() const
{
	Goals::TGoalVec tasks;

	auto captureObjects = [&](const std::vector<const CGObjectInstance*> & objs) -> void{
		if(objs.empty())
		{
			return;
		}

		logAi->debug("Scanning objects, count %d", objs.size());

		for(auto objToVisit : objs)
		{			
#if AI_TRACE_LEVEL >= 1
			logAi->trace("Checking object %s, %s", objToVisit->getObjectName(), objToVisit->visitablePos().toString());
#endif

			if(!shouldVisitObject(objToVisit))
				continue;

			const int3 pos = objToVisit->visitablePos();

			auto paths = ai->ah->getPathsToTile(pos);
			std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;
			std::shared_ptr<ExecuteHeroChain> closestWay;
					
#if AI_TRACE_LEVEL >= 1
			logAi->trace("Found %d paths", paths.size());
#endif

			for(auto & path : paths)
			{
#if AI_TRACE_LEVEL >= 2
				logAi->trace("Path found %s", path.toString());
#endif

				if(ai->nullkiller->dangerHitMap->enemyCanKillOurHeroesAlongThePath(path))
				{
#if AI_TRACE_LEVEL >= 2
					logAi->trace("Ignore path. Target hero can be killed by enemy. Our power %lld", path.heroArmy->getArmyStrength());
#endif
					continue;
				}

				if(!shouldVisit(path.targetHero, objToVisit))
					continue;

				auto hero = path.targetHero;
				auto danger = path.getTotalDanger();

				if(ai->ah->getHeroRole(hero) == HeroRole::SCOUT && danger == 0 && path.exchangeCount > 1)
					continue;

				auto firstBlockedAction = path.getFirstBlockedAction();
				if(firstBlockedAction)
				{
					auto subGoal = firstBlockedAction->decompose(path.targetHero);

#if AI_TRACE_LEVEL >= 2
					logAi->trace("Decomposing special action %s returns %s", firstBlockedAction->toString(), subGoal->toString());
#endif

					if(!subGoal->invalid())
					{
						Composition composition;

						composition.addNext(ExecuteHeroChain(path, objToVisit));
						composition.addNext(subGoal);

						tasks.push_back(sptr(composition));
					}

					continue;
				}

				auto isSafe = isSafeToVisit(hero, path.heroArmy, danger);
				
#if AI_TRACE_LEVEL >= 2
				logAi->trace(
					"It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld", 
					isSafe ? "safe" : "not safe",
					objToVisit->getObjectName(), 
					hero->name,
					path.getHeroStrength(),
					danger,
					path.getTotalArmyLoss());
#endif

				if(isSafe)
				{
					auto newWay = std::make_shared<ExecuteHeroChain>(path, objToVisit);

					waysToVisitObj.push_back(newWay);

					if(!closestWay || closestWay->getPath().movementCost() > newWay->getPath().movementCost())
						closestWay = newWay;
				}
			}

			if(waysToVisitObj.empty())
				continue;

			for(auto way : waysToVisitObj)
			{
				if(ai->nullkiller->arePathHeroesLocked(way->getPath()))
					continue;

				way->closestWayRatio
					= closestWay->getPath().movementCost() / way->getPath().movementCost();

				tasks.push_back(sptr(*way));
			}
		}
	};

	if(specificObjects)
	{
		captureObjects(objectsToCapture);
	}
	else
	{
		captureObjects(std::vector<const CGObjectInstance*>(ai->visitableObjs.begin(), ai->visitableObjs.end()));
	}

	return tasks;
}

bool CaptureObjectsBehavior::shouldVisitObject(ObjectIdRef obj) const
{
	const CGObjectInstance* objInstance = obj;
	if(!objInstance)
		return false;

	if(objectTypes.size() && !vstd::contains(objectTypes, objInstance->ID.num))
	{
		return false;
	}

	if(objectSubTypes.size() && !vstd::contains(objectSubTypes, objInstance->subID))
	{
		return false;
	}
	
	if(isObjectRemovable(obj))
	{
		return true;
	}

	const int3 pos = objInstance->visitablePos();

	if(objInstance->ID != Obj::CREATURE_GENERATOR1 && vstd::contains(ai->alreadyVisited, objInstance)
		|| obj->wasVisited(ai->playerID))
	{
		return false;
	}

	auto playerRelations = cb->getPlayerRelations(ai->playerID, objInstance->tempOwner);
	if(playerRelations != PlayerRelations::ENEMIES && !isWeeklyRevisitable(objInstance))
	{
		return false;
	}

	//it may be hero visiting this obj
	//we don't try visiting object on which allied or owned hero stands
	// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
	const CGObjectInstance * topObj = cb->getTopObj(pos);

	if(!topObj)
		return false; // partly visible obj but its visitable pos is not visible.

	if(topObj->ID == Obj::HERO && cb->getPlayerRelations(ai->playerID, topObj->tempOwner) != PlayerRelations::ENEMIES)
		return false;
	else
		return true; //all of the following is met
}
