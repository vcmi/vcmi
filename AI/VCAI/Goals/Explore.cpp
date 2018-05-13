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
#include "Goals/Explore.h"
#include "Goals/CaptureObjects.h"
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

Tasks::TaskList Explore::getTasks() {
	Tasks::TaskList tasks = Tasks::TaskList();

	if (!hero->movement) {
		return tasks;
	}

	addTasks(tasks, sptr(CaptureObjects(this->getExplorationHelperObjects()).sethero(hero)), 0.8);

	int3 t = whereToExplore(hero);
	if (t.valid())
	{
		addTask(tasks, Tasks::VisitTile(t, hero), 0.5);
	}
	else
	{
		t = ai->explorationDesperate(hero);

		if (t.valid()) //don't waste time if we are completely blocked
			addTasks(tasks, sptr(Goals::ClearWayTo(t, hero)), 0); // TODO: Implement ClearWayTo.getTasks
	}

	return tasks;
}

std::vector<const CGObjectInstance *> Explore::getExplorationHelperObjects() {
	//try to use buildings that uncover map
	std::vector<const CGObjectInstance *> objs;
	for (auto obj : ai->visitableObjs)
	{
		if (!vstd::contains(ai->alreadyVisited, obj))
		{
			switch (obj->ID.num)
			{
			case Obj::REDWOOD_OBSERVATORY:
			case Obj::PILLAR_OF_FIRE:
			case Obj::CARTOGRAPHER:
				objs.push_back(obj);
				break;
			case Obj::MONOLITH_ONE_WAY_ENTRANCE:
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				assert(ai->knownTeleportChannels.find(tObj->channel) != ai->knownTeleportChannels.end());
				if (TeleportChannel::IMPASSABLE != ai->knownTeleportChannels[tObj->channel]->passability)
					objs.push_back(obj);
				break;
			}
		}
		else
		{
			switch (obj->ID.num)
			{
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				if (TeleportChannel::IMPASSABLE == ai->knownTeleportChannels[tObj->channel]->passability)
					break;
				for (auto exit : ai->knownTeleportChannels[tObj->channel]->exits)
				{
					if (!cb->getObj(exit))
					{ // Always attempt to visit two-way teleports if one of channel exits is not visible
						objs.push_back(obj);
						break;
					}
				}
				break;
			}
		}
	}

	return objs;
}