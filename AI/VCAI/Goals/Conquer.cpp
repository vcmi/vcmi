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
#include "Conquer.h"
#include "Build.h"
#include "Defence.h"
#include "Explore.h"
#include "CaptureObjects.h"
#include "GatherArmy.h"
#include "VisitNearestTown.h"
#include "../VCAI.h"
#include "../SectorMap.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Tasks/VisitTile.h"
#include "../Tasks/BuildStructure.h"
#include "../Tasks/RecruitHero.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;

std::map<const CGHeroInstance*, const CGTownInstance*> getHeroTownMap(std::vector<const CGHeroInstance*> heroes) {
	auto towns = cb->getTownsInfo();
	std::map<const CGHeroInstance*, const CGTownInstance*> result;

	if (heroes.size() <= 1 || towns.empty()) {
		return result;
	}

	vstd::erase_if_present(heroes, heroes.at(0));

	for (const CGTownInstance* town : towns) {
		auto hero = getNearestHero(heroes, town->visitablePos());
		vstd::erase_if_present(heroes, hero);

		result[hero] = town;

		if (heroes.empty()) {
			break;
		}
	}

	return result;
}

Tasks::TaskList Conquer::getTasks() {
	auto tasks = Tasks::TaskList();
	auto heroes = cb->getHeroesInfo();

	// lets process heroes according their army strength in descending order
	std::sort(heroes.begin(), heroes.end(), isLevelHigher);

	addTasks(tasks, sptr(Defence()), 1);

	if (tasks.size()) {
		return tasks;
	}

	addTasks(tasks, sptr(CaptureObjects().ofType(Obj::HERO)), 1);
	addTasks(tasks, sptr(CaptureObjects().ofType(Obj::TOWN)), 0.95);

	if (tasks.size()) {
		sortByPriority(tasks);

		return tasks;
	}

	addTasks(tasks, sptr(Build()), 0.8);
	addTasks(tasks, sptr(RecruitHero()));

	if (tasks.size()) {
		sortByPriority(tasks);

		return tasks;
	}

	addTasks(tasks, sptr(GatherArmy()), 0.7); // no hero - just pickup existing army, no buy

	auto heroTownMap = getHeroTownMap(heroes);

	for (auto nextHero : heroes) {
		if (!nextHero->movement) {
			continue;
		}

		auto heroPtr = HeroPtr(nextHero);
		auto heroTasks = Tasks::TaskList();

		logAi->trace("Considering tasks for hero %s", nextHero->name);

		addTasks(heroTasks, sptr(CaptureObjects().ofType(Obj::MINE).sethero(heroPtr)), 0.58);
		addTasks(heroTasks, sptr(CaptureObjects().sethero(HeroPtr(heroPtr))), 0.5);

		const CGHeroInstance* strongestHero = heroes.at(0);

		if (cb->getDate(Date::DAY) > 21 && nextHero == strongestHero) {
			addTasks(heroTasks, sptr(Explore().sethero(HeroPtr(heroPtr))), 0.65);
		}
		else {
			if (vstd::contains(heroTownMap, nextHero)) {
				auto assignedTown = heroTownMap.at(nextHero);
				if (assignedTown->hasBuilt(BuildingID::CITY_HALL) && !assignedTown->visitingHero) {
					addTask(heroTasks, Tasks::VisitTile(assignedTown->visitablePos(), nextHero, assignedTown), 0.3);
				}
			}
			else {
				addTasks(heroTasks, sptr(Explore().sethero(HeroPtr(heroPtr))), 0.5);
			}
		}

		if (heroTasks.empty() && nextHero->movement > 0) {
			// sometimes there is nothing better than go to the nearest town
			addTasks(heroTasks, sptr(VisitNearestTown().sethero(heroPtr)), 0);
		}

		if (heroTasks.size()) {
			sortByPriority(heroTasks);

			for (auto task : tasks) {
				logAi->trace("found task %s, priority %.3f", task->toString(), task->getPriority());
			}

			tasks.push_back(heroTasks.front());

			break;
		}
	}

	sortByPriority(tasks);
	
	return tasks;
}
