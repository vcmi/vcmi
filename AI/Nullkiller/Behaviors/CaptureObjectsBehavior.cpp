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
#include "../Goals/ExecuteHeroChain.h"
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
			auto paths = ai->ah->getPathsToTile(pos);
			std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;
			std::shared_ptr<ExecuteHeroChain> closestWay;

			for(auto & path : paths)
			{
#ifdef VCMI_TRACE_PATHFINDER
				std::stringstream str;

				str << "Path found ";

				for(auto node : path.nodes)
					str << node.targetHero->name << "->" << node.coord.toString() << "; ";

				logAi->trace(str.str());
#endif

				if(!shouldVisit(path.targetHero, objToVisit))
					continue; 
				
				auto hero = path.targetHero;
				auto danger = path.getTotalDanger(hero);

				if(isSafeToVisit(hero, path.heroArmy, danger))
				{
					auto newWay = std::make_shared<ExecuteHeroChain>(path, objToVisit);

					waysToVisitObj.push_back(newWay);

					if(!closestWay || closestWay->evaluationContext.movementCost > newWay->evaluationContext.movementCost)
						closestWay = newWay;
				}
			}
			
			if(waysToVisitObj.empty())
				continue;
			
			for(auto way : waysToVisitObj)
			{
				way->evaluationContext.closestWayRatio 
					= way->evaluationContext.movementCost / closestWay->evaluationContext.movementCost;

				logAi->trace("Behavior %s found %s(%s), danger %d", toString(), way->name(), way->tile.toString(), way->evaluationContext.danger);
		
				tasks.push_back(sptr(*way));
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
