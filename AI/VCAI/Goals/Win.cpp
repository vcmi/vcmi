/*
* Win.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapping/CMapHeader.h" //for victory conditions
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/constants/StringConstants.h"

using namespace Goals;

TSubgoal Win::whatToDoToAchieve()
{
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

	for(const TriggeredEvent & event : cb->getMapHeader()->triggeredEvents)
	{
		//TODO: try to eliminate human player(s) using loss conditions that have isHuman element

		if(event.effect.type == EventEffect::VICTORY)
		{
			boost::range::copy(event.trigger.getFulfillmentCandidates(toBool), std::back_inserter(goals));
		}
	}

	//TODO: instead of returning first encountered goal AI should generate list of possible subgoals
	for(const EventCondition & goal : goals)
	{
		switch(goal.condition)
		{
		case EventCondition::HAVE_ARTIFACT:
			return sptr(GetArtOfType(goal.objectType));
		case EventCondition::DESTROY:
		{
			if(goal.object)
			{
				auto obj = cb->getObj(goal.object->id);
				if(obj)
					if(obj->getOwner() == ai->playerID) //we can't capture our own object
						return sptr(Conquer());


				return sptr(VisitObj(goal.object->id.getNum()));
			}
			else
			{
				// TODO: destroy all objects of type goal.objectType
				// This situation represents "kill all creatures" condition from H3
				break;
			}
		}
		case EventCondition::HAVE_BUILDING:
		{
			// TODO build other buildings apart from Grail
			// goal.objectType = buidingID to build
			// goal.object = optional, town in which building should be built
			// Represents "Improve town" condition from H3 (but unlike H3 it consists from 2 separate conditions)

			if(goal.objectType == BuildingID::GRAIL)
			{
				if(auto h = ai->getHeroWithGrail())
				{
					//hero is in a town that can host Grail
					if(h->visitedTown && !vstd::contains(h->visitedTown->forbiddenBuildings, BuildingID::GRAIL))
					{
						const CGTownInstance * t = h->visitedTown;
						return sptr(BuildThis(BuildingID::GRAIL, t).setpriority(10));
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
						if(towns.size())
						{
							return sptr(VisitTile(towns.front()->visitablePos()).sethero(h));
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
				if(ratio > 0.99)
				{
					return sptr(DigAtTile(grailPos));
				} //TODO: use FIND_OBJ
				else if(const CGObjectInstance * obj = ai->getUnvisitedObj(objWithID<Obj::OBELISK>)) //there are unvisited Obelisks
					return sptr(VisitObj(obj->id.getNum()));
				else
					return sptr(Explore());
			}
			break;
		}
		case EventCondition::CONTROL:
		{
			if(goal.object)
			{
				auto objRelations = cb->getPlayerRelations(ai->playerID, goal.object->tempOwner);
				
				if(objRelations == PlayerRelations::ENEMIES)
				{
					return sptr(VisitObj(goal.object->id.getNum()));
				}
				else
				{
					// TODO: Defance
					break;
				}
			}
			else
			{
				//TODO: control all objects of type "goal.objectType"
				// Represents H3 condition "Flag all mines"
				break;
			}
		}

		case EventCondition::HAVE_RESOURCES:
			//TODO mines? piles? marketplace?
			//save?
			return sptr(CollectRes(static_cast<EGameResID>(goal.objectType), goal.value));
		case EventCondition::HAVE_CREATURES:
			return sptr(GatherTroops(goal.objectType, goal.value));
		case EventCondition::TRANSPORT:
		{
			//TODO. merge with bring Grail to town? So AI will first dig grail, then transport it using this goal and builds it
			// Represents "transport artifact" condition:
			// goal.objectType = type of artifact
			// goal.object = destination-town where artifact should be transported
			break;
		}
		case EventCondition::STANDARD_WIN:
			return sptr(Conquer());

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
			//TODO: support new condition format
			return sptr(Conquer());
		default:
			assert(0);
		}
	}
	return sptr(Invalid());
}
