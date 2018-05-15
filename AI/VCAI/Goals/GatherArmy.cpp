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
#include "Goals/GatherArmy.h"
#include "VCAI.h"
#include "Fuzzy.h"
#include "SectorMap.h"
#include "../../lib/mapping/CMap.h" //for victory conditions
#include "../../lib/CPathfinder.h"
#include "Tasks/VisitTile.h"
#include "Tasks/BuildStructure.h"
#include "Tasks/RecruitHero.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;

Tasks::TaskList GatherArmy::getTasks() {
	Tasks::TaskList tasks;

	auto heroes = cb->getHeroesInfo();
	auto towns = cb->getTownsInfo();

	if (!this->hero) {
		if (heroes.empty()) {
			return tasks;
		}

		for (auto town : towns) {
			if (!town->getArmyStrength()) {
				continue;
			}

			int3 pos = town->visitablePos();
			std::vector<const CGHeroInstance*> copy = heroes;

			vstd::erase_if(copy, [town](const CGHeroInstance* h) -> bool {
				return howManyReinforcementsCanGet(h, town) < h->getArmyStrength() / 3;
			});

			if (copy.empty()) {
				continue;
			}

			auto carrier = nearestHero(copy, pos);

			addTask(tasks, Tasks::VisitTile(pos, carrier), 1);
		}

		return tasks;
	}


	auto targetHeroPosition = this->hero->visitablePos();

	for (HeroPtr hero : heroes) {
		auto isStronger = hero->getFightingStrength() > this->hero->getFightingStrength();
		auto isAccessible = ai->isAccessibleForHero(targetHeroPosition, hero, true);

		if (hero.h == this->hero.h
			|| isStronger
			|| !isAccessible
			|| !ai->canGetArmy(this->hero.get(), hero.get())) {
			continue;
		}

		addTask(tasks, Tasks::VisitTile(targetHeroPosition, hero), 1);

		break;
	}
	//TODO take town if it is closer than hero
	boost::sort(towns, CDistanceSorter(this->hero.get()));

	for (auto town : towns) {
		auto pos = town->visitablePos();
		auto pathInfo = cb->getPathsInfo(this->hero.get())->getPathInfo(pos);

		if (pathInfo->reachable()) {
			ai->buildArmyIn(town);
		}

		if (howManyReinforcementsCanGet(hero, town))
		{
			// TODO: invoke army pickup logic from above
			break;
		}
	}

	return tasks;
}
