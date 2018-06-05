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
#include "Defence.h"
#include "Build.h"
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

Tasks::TaskList Defence::getTasks() {
	auto tasks = Tasks::TaskList();
	auto enemyHeroes = cb->getHeroesInfo(false);
	auto myHeroes = cb->getHeroesInfo();
	auto myColor = cb->getMyColor().get();

	vstd::erase_if(enemyHeroes, [myColor](const CGHeroInstance* h) -> bool {
		return cb->getPlayerRelations(h->tempOwner, myColor) != PlayerRelations::ENEMIES;
	});

	if (enemyHeroes.empty()) {
		return tasks;
	}

	if (myHeroes.empty()) {
		//addTask(tasks, Tasks::RecruitHero(), 0);
		return tasks;
	}

	// lets process heroes according their army strength in descending order
	std::sort(myHeroes.begin(), myHeroes.end(), isLevelHigher);
	std::sort(enemyHeroes.rbegin(), enemyHeroes.rend(), compareHeroStrength);

	auto ourStrongestHero = myHeroes.at(0);
	auto enemyStrongestHero = enemyHeroes.at(0);

	if (!ai->isAccessibleForHero(enemyStrongestHero->visitablePos(), ourStrongestHero)) {
		return tasks;
	}

	si64 heroStrength = ourStrongestHero->getTotalStrength();
	si64 dangerStrength = evaluateDanger(enemyStrongestHero->visitablePos(), ourStrongestHero);

	if (dangerStrength > heroStrength * SAFE_ATTACK_CONSTANT) {
		// we are weeker, defence
		addTasks(tasks, sptr(CaptureObjects(enemyStrongestHero).withForce().sethero(ourStrongestHero)), 0.9);

		addTasks(tasks, sptr(Build(false)), 0.1);
		ai->saving = 0;

		addTasks(tasks, sptr(GatherArmy()), 0.8); // no hero - just pickup existing army, no buy

		for (auto nextHero : myHeroes) {
			if (tasks.size()) {
				return tasks;
			}

			addTasks(tasks, sptr(VisitNearestTown().sethero(nextHero)), 0);
		}

		if (tasks.empty()) {
			throw std::exception("Defence: Can not do anything useful. Lets end turn and just wait.");
		}
	}
	else if (dangerStrength > heroStrength / 2) {
		addTask(tasks, Tasks::VisitTile(enemyStrongestHero->visitablePos(), ourStrongestHero, enemyStrongestHero), 1);
	}

	sortByPriority(tasks);
	
	return tasks;
}
