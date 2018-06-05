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
#include "Win.h"
#include "Conquer.h"
#include "CaptureObjects.h"
#include "../VCAI.h"
#include "../Fuzzy.h"
#include "../SectorMap.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Tasks/VisitTile.h"
#include "../Tasks/BuildStructure.h"
#include "../Tasks/RecruitHero.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;

Tasks::TaskList Win::getTasks() {
	Tasks::TaskList tasks;

	addTasks(tasks, sptr(Conquer()), 0.5);

	auto toBool = [=](const EventCondition &)
	{
		// TODO: proper implementation
		// Right now even already fulfilled goals will be included into generated list
		// Proper check should test if event condition is already fulfilled
		// Easiest way to do this is to call CGameState::checkForVictory but this function should not be
		// used on client side or in AI code
		return false;
	};

	std::vector<EventCondition> goals;

	for (const TriggeredEvent & event : cb->getMapHeader()->triggeredEvents)
	{
		//TODO: try to eliminate human player(s) using loss conditions that have isHuman element

		if (event.effect.type == EventEffect::VICTORY)
		{
			boost::range::copy(event.trigger.getFulfillmentCandidates(toBool), std::back_inserter(goals));
		}
	}

	//TODO: instead of returning first encountered goal AI should generate list of possible subgoals
	for (const EventCondition & goal : goals)
	{
		switch (goal.condition)
		{
		case EventCondition::HAVE_ARTIFACT:
			addTasks(tasks, sptr(Goals::CaptureObjects().ofType(goal.objectType)), 1);
			break;
		case EventCondition::DESTROY:
		case EventCondition::CONTROL:
			if (goal.object)
			{
				auto obj = cb->getObj(goal.object->id);
				if (obj)
					if (obj->getOwner() == ai->playerID) //we can't capture our own object
						break;
				std::vector<const CGObjectInstance*> objectsToCapture;
				objectsToCapture.push_back(obj);

				addTasks(tasks, sptr(Goals::CaptureObjects(objectsToCapture)), 1);
			}
			else
			{
				addTasks(tasks, sptr(Goals::CaptureObjects().ofType(goal.objectType)), 1);
			}
			break;
		case EventCondition::HAVE_BUILDING:
		{
			// TODO build other buildings apart from Grail
			// goal.objectType = buidingID to build
			// goal.object = optional, town in which building should be built
			// Represents "Improve town" condition from H3 (but unlike H3 it consists from 2 separate conditions)

			if (goal.objectType == BuildingID::GRAIL)
			{
				if (auto h = ai->getHeroWithGrail())
				{
					//hero is in a town that can host Grail
					if (h->visitedTown && !vstd::contains(h->visitedTown->forbiddenBuildings, BuildingID::GRAIL))
					{
						const CGTownInstance * t = h->visitedTown;
						addTask(tasks, Tasks::BuildStructure(BuildingID::GRAIL, t), 1);
					}
					else
					{
						auto towns = cb->getTownsInfo();
						towns.erase(boost::remove_if(towns,
							[](const CGTownInstance * t) -> bool
						{
							return vstd::contains(t->forbiddenBuildings, BuildingID::GRAIL);
						}),
							towns.end());
						boost::sort(towns, CDistanceSorter(h.get()));
						if (towns.size())
						{
							addTask(tasks, Tasks::VisitTile(towns.front()->visitablePos(), h, town), 1);
						}
					}
				}
				double ratio = 0;
				// maybe make this check a bit more complex? For example:
				// 0.75 -> dig randomly within 3 tiles radius
				// 0.85 -> radius now 2 tiles
				// 0.95 -> 1 tile radius, position is fully known
				// AFAIK H3 AI does something like this
				int3 grailPos = cb->getGrailPos(&ratio);
				if (ratio > 0.99)
				{
					addTasks(tasks, sptr(Goals::DigAtTile(grailPos)), 1);
				} //TODO: use FIND_OBJ
				else {
					addTasks(tasks, sptr(CaptureObjects().ofType(Obj::OBELISK)), 1);
				}
			}
			break;
		}

		case EventCondition::HAVE_RESOURCES:
			addTasks(tasks, sptr(Goals::GetResources(static_cast<Res::ERes>(goal.objectType), goal.value)), 1);
			break;
		case EventCondition::HAVE_CREATURES:
			addTasks(tasks, sptr(Goals::GatherTroops(goal.objectType, goal.value)), 1);
			break;
		case EventCondition::TRANSPORT:
		{
			//TODO. merge with bring Grail to town? So AI will first dig grail, then transport it using this goal and builds it
			// Represents "transport artifact" condition:
			// goal.objectType = type of artifact
			// goal.object = destination-town where artifact should be transported
			break;
		}
		case EventCondition::STANDARD_WIN:
			break;
			// Conditions that likely don't need any implementation
		case EventCondition::DAYS_PASSED:
			break; // goal.value = number of days for condition to trigger
		case EventCondition::DAYS_WITHOUT_TOWN:
			break; // goal.value = number of days to trigger this
		case EventCondition::IS_HUMAN:
			break; // Should be only used in calculation of candidates (see toBool lambda)
		case EventCondition::CONST_VALUE:
			break;

		case EventCondition::HAVE_0:
		case EventCondition::HAVE_BUILDING_0:
		case EventCondition::DESTROY_0:
			break;
		default:
			assert(0);
		}
	}

	return tasks;
}
