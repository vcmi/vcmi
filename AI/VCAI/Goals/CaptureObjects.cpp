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
#include "Goals/CaptureObjects.h"
#include "Goals/GatherArmy.h"
#include "VCAI.h"
#include "AIUtility.h"
#include "SectorMap.h"
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"
#include "Tasks/VisitTile.h"
#include "Tasks/BuildStructure.h"
#include "Tasks/RecruitHero.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;

std::string CaptureObjects::toString() const {
	return "Capture objects";
}

Tasks::TaskList CaptureObjects::getTasks() {
	Tasks::TaskList tasks;

	auto needArmy = false;
	auto heroes = cb->getHeroesInfo();
	
	auto captureObjects = [&](std::vector<const CGObjectInstance*> objs, HeroPtr hero) -> bool {
		if (objs.empty()) {
			return false;
		}

		auto sm = ai->getCachedSectorMap(hero);
		auto pathsInfo = cb->getPathsInfo(hero.get());

		boost::sort(objs, CDistanceSorter(hero.get()));

		for (auto objToVisit : objs) {
			const int3 pos = objToVisit->visitablePos();
			const int3 targetPos = sm->firstTileToGet(hero, pos);

			if (!this->shouldVisitObject(objToVisit, hero, *sm)) {
				continue;
			}

			if (targetPos.x == -1) {
				return false;
			}

			if (isSafeToVisit(hero, targetPos)) {
				auto nearestHero = getNearestHero(heroes, targetPos);

				if (nearestHero != hero.get() && isSafeToVisit(nearestHero, targetPos)) {
					continue;
				}

				auto pathInfo = pathsInfo->getPathInfo(targetPos);
				double priority = 0;

				if (pathInfo->turns == 0) {
					priority = 1 - (hero->movement - pathInfo->moveRemains) / (double)hero->maxMovePoints(true);
				}

				addTask(tasks, Tasks::VisitTile(targetPos, hero, objToVisit), priority);

				return true;
			}
			else {
				needArmy = true;
			}
		}

		return false;
	};

	if (specificObjects) {
		if (this->hero) {
			captureObjects(objectsToCapture, this->hero);
		}
		else {
			for (auto hero : heroes) {
				captureObjects(objectsToCapture, hero);
			}
		}
	}
	else {
		auto sm = ai->getCachedSectorMap(hero);

		if (!captureObjects(sm->getNearbyObjs(hero, 1), this->hero)) {
			captureObjects(std::vector<const CGObjectInstance*>(ai->visitableObjs.begin(), ai->visitableObjs.end()), this->hero);
		}
	}

	if (!tasks.size() && this->hero && needArmy) {
		addTasks(tasks, sptr(GatherArmy().sethero(this->hero)));
	}

	return tasks;
}

bool CaptureObjects::shouldVisitObject(ObjectIdRef obj, HeroPtr hero, SectorMap& sm) {
	const CGObjectInstance* objInstance = obj;

	if (!objInstance || objectTypes.size() && !vstd::contains(objectTypes, objInstance->ID.num)) {
		return false;
	}

	const int3 pos = objInstance->visitablePos();
	const int3 targetPos = sm.firstTileToGet(hero, pos);

	if (!targetPos.valid()
		|| vstd::contains(ai->alreadyVisited, objInstance)) {
		return false;
	}

	if (objInstance->wasVisited(hero.get())) {
		return false;
	}

	auto playerRelations = cb->getPlayerRelations(ai->playerID, objInstance->tempOwner);
	if (playerRelations != PlayerRelations::ENEMIES && !isWeeklyRevisitable(objInstance)) {
		return false;
	}

	if (!shouldVisit(hero, objInstance)) {
		return false;
	}

	if (!ai->isAccessibleForHero(targetPos, hero))
	{
		return false;
	}

	//it may be hero visiting this obj
	//we don't try visiting object on which allied or owned hero stands
	// -> it will just trigger exchange windows and AI will be confused that obj behind doesn't get visited
	const CGObjectInstance * topObj = cb->getTopObj(obj->visitablePos());

	if (topObj->ID == Obj::HERO && cb->getPlayerRelations(hero->tempOwner, topObj->tempOwner) != PlayerRelations::ENEMIES)
		return false;
	else
		return true; //all of the following is met
}
