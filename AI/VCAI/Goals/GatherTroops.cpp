/*
* GatherTroops.cpp, part of VCMI engine
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
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool GatherTroops::operator==(const GatherTroops & other) const
{
	return objid == other.objid;
}

int GatherTroops::getCreaturesCount(const CArmedInstance * army)
{
	int count = 0;

	for(auto stack : army->Slots())
	{
		if(objid == stack.second->getCreatureID().num)
		{
			count += stack.second->count;
		}
	}

	return count;
}

TSubgoal GatherTroops::whatToDoToAchieve()
{
	logAi->trace("Entering GatherTroops::whatToDoToAchieve");

	auto heroes = cb->getHeroesInfo(true);

	for(auto hero : heroes)
	{
		if(getCreaturesCount(hero) >= this->value)
		{
			logAi->trace("Completing GATHER_TROOPS by hero %s", hero->name);

			throw goalFulfilledException(sptr(*this));
		}
	}

	TGoalVec solutions = getAllPossibleSubgoals();

	if(solutions.empty())
		return sptr(Explore());

	return fh->chooseSolution(solutions);
}


TGoalVec GatherTroops::getAllPossibleSubgoals()
{
	TGoalVec solutions;

	for(const CGTownInstance * t : cb->getTownsInfo())
	{
		int count = getCreaturesCount(t->getUpperArmy());

		if(count >= this->value)
		{
			if(t->visitingHero)
			{
				solutions.push_back(sptr(VisitObj(t->id.getNum()).sethero(t->visitingHero.get())));
			}
			else
			{
				vstd::concatenate(solutions, ai->ah->howToVisitObj(t));
			}

			continue;
		}

		auto creature = VLC->creh->objects[objid];
		if(t->subID == creature->faction) //TODO: how to force AI to build unupgraded creatures? :O
		{
			auto creatures = vstd::tryAt(t->town->creatures, creature->level - 1);
			if(!creatures)
				continue;

			int upgradeNumber = vstd::find_pos(*creatures, creature->idNumber);
			if(upgradeNumber < 0)
				continue;

			BuildingID bid(BuildingID::DWELL_FIRST + creature->level - 1 + upgradeNumber * GameConstants::CREATURES_PER_TOWN);
			if(t->hasBuilt(bid) && ai->ah->freeResources().canAfford(creature->cost)) //this assumes only creatures with dwellings are assigned to faction
			{
				solutions.push_back(sptr(BuyArmy(t, creature->AIValue * this->value).setobjid(objid)));
			}
			/*else //disable random building requests for now - this code needs to know a lot of town/resource context to do more good than harm
			{
				return sptr(BuildThis(bid, t).setpriority(priority));
			}*/
		}
	}

	for(auto obj : ai->visitableObjs)
	{
		auto d = dynamic_cast<const CGDwelling *>(obj);

		if(!d || obj->ID == Obj::TOWN)
			continue;

		for(auto creature : d->creatures)
		{
			if(creature.first) //there are more than 0 creatures avaliabe
			{
				for(auto type : creature.second)
				{
					if(type == objid && ai->ah->freeResources().canAfford(VLC->creh->objects[type]->cost))
						vstd::concatenate(solutions, ai->ah->howToVisitObj(obj));
				}
			}
		}
	}

	CreatureID creID = CreatureID(objid);

	vstd::erase_if(solutions, [&](TSubgoal goal)->bool
	{
		return goal->hero && !goal->hero->getSlotFor(creID).validSlot() && !goal->hero->getFreeSlot().validSlot();
	});

	return solutions;
	//TODO: exchange troops between heroes
}

bool GatherTroops::fulfillsMe(TSubgoal goal)
{
	if (!hero || hero == goal->hero) //we got army for desired hero or any hero
		if (goal->objid == objid) //same creature type //TODO: consider upgrades?
			if (goal->value >= value) //notify every time we get resources?
				return true;
	return false;
}
