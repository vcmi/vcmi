/*
 * Goals.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../VCAI.h"
#include "../AIhelper.h"
#include "CaptureObjectsBehavior.h"
#include "../AIUtility.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

std::string CaptureObjectsBehavior::toString() const {
	return "Capture objects";
}

Goals::TGoalVec CaptureObjectsBehavior::getTasks() {
	Goals::TGoalVec tasks;
	
	auto captureObjects = [&](std::vector<const CGObjectInstance*> objs) -> void {
		if (objs.empty()) {
			return;
		}

		for (auto objToVisit : objs) {
			if(!shouldVisitObject(objToVisit))
				continue;

			const int3 pos = objToVisit->visitablePos();
			Goals::TGoalVec waysToVisitObj = ai->ah->howToVisitObj(objToVisit, false);

			vstd::erase_if(waysToVisitObj, [objToVisit](Goals::TSubgoal goal) -> bool
			{
				return !goal->hero.validAndSet() 
					|| !shouldVisit(goal->hero, objToVisit)
					|| goal->evaluationContext.danger * 1.5 > goal->hero->getTotalStrength();
			});

			if(waysToVisitObj.empty())
				continue;

			Goals::TSubgoal closestWay = *vstd::minElementByFun(waysToVisitObj, [](Goals::TSubgoal goal) -> float {
				return goal->evaluationContext.movementCost;
			});

			for(Goals::TSubgoal way : waysToVisitObj)
			{
				if(!way->hero->movement)
					continue;

				way->evaluationContext.closestWayRatio 
					= way->evaluationContext.movementCost / closestWay->evaluationContext.movementCost;

				logAi->trace("Behavior %s found %s(%s), danger %d", toString(), way->name(), way->tile.toString(), way->evaluationContext.danger);

				tasks.push_back(way);
			}
		}
	};

	if (specificObjects) {
		captureObjects(objectsToCapture);
	}
	else {
		captureObjects(std::vector<const CGObjectInstance*>(ai->visitableObjs.begin(), ai->visitableObjs.end()));
	}

	return tasks;
}

bool CaptureObjectsBehavior::shouldVisitObject(ObjectIdRef obj) const
{
	const CGObjectInstance* objInstance = obj;

	if (!objInstance || objectTypes.size() && !vstd::contains(objectTypes, objInstance->ID.num)) {
		return false;
	}

	if (!objInstance || objectSubTypes.size() && !vstd::contains(objectSubTypes, objInstance->subID)) {
		return false;
	}

	const int3 pos = objInstance->visitablePos();

	if (vstd::contains(ai->alreadyVisited, objInstance)) {
		return false;
	}

	auto playerRelations = cb->getPlayerRelations(ai->playerID, objInstance->tempOwner);
	if (playerRelations != PlayerRelations::ENEMIES && !isWeeklyRevisitable(objInstance)) {
		return false;
	}

	//it may be hero visiting this obj
	//we don't try visiting object on which allied or owned hero stands
	// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
	const CGObjectInstance * topObj = cb->getTopObj(obj->visitablePos());

	if (topObj->ID == Obj::HERO && cb->getPlayerRelations(ai->playerID, topObj->tempOwner) != PlayerRelations::ENEMIES)
		return false;
	else
		return true; //all of the following is met
}
