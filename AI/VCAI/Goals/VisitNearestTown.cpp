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
#include "VisitNearestTown.h"
#include "../VCAI.h"
#include "../SectorMap.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Tasks/VisitTile.h"

extern boost::thread_specific_ptr<CCallback> cb;

using namespace Goals;

Tasks::TaskList VisitNearestTown::getTasks() {
	Tasks::TaskList tasks;

	auto towns = cb->getTownsInfo();
	auto alreadyInTown = vstd::contains_if(towns, [this](const CGTownInstance* town) -> bool {
		return town->visitingHero.get() == hero.get();
	});

	if (alreadyInTown) {
		return tasks;
	}

	vstd::erase_if(towns, [](const CGTownInstance* t) -> bool {return t->visitingHero.get(); });

	if (towns.size()) {
		boost::sort(towns, CDistanceSorter(this->hero.get()));
		auto target = towns.at(0);

		addTask(tasks, Tasks::VisitTile(target->visitablePos(), this->hero, target));
	}

	return tasks;
}
