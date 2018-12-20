/*
* Explore.cpp, part of VCMI engine
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

bool Explore::operator==(const Explore & other) const
{
	return other.hero.h == hero.h;
}

std::string Explore::completeMessage() const
{
	return "Hero " + hero.get()->name + " completed exploration";
}

TSubgoal Explore::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());
	if(hero) //use best step for this hero
	{
		return ret;
	}
	else
	{
		if(ret->hero.get(true))
			return sptr(Explore().sethero(ret->hero.h)); //choose this hero and then continue with him
		else
			return ret; //other solutions, like buying hero from tavern
	}
}

TGoalVec Explore::getAllPossibleSubgoals()
{
	TGoalVec ret;
	std::vector<const CGHeroInstance *> heroes;

	if(hero)
	{
		heroes.push_back(hero.h);
	}
	else
	{
		//heroes = ai->getUnblockedHeroes();
		heroes = cb->getHeroesInfo();
		vstd::erase_if(heroes, [](const HeroPtr h)
		{
			if(ai->getGoal(h)->goalType == EXPLORE) //do not reassign hero who is already explorer
				return true;

			if(!ai->isAbleToExplore(h))
				return true;

			return !h->movement; //saves time, immobile heroes are useless anyway
		});
	}

	//try to use buildings that uncover map
	std::vector<const CGObjectInstance *> objs;
	for(auto obj : ai->visitableObjs)
	{
		if(!vstd::contains(ai->alreadyVisited, obj))
		{
			switch(obj->ID.num)
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
				if(TeleportChannel::IMPASSABLE != ai->knownTeleportChannels[tObj->channel]->passability)
					objs.push_back(obj);
				break;
			}
		}
		else
		{
			switch(obj->ID.num)
			{
			case Obj::MONOLITH_TWO_WAY:
			case Obj::SUBTERRANEAN_GATE:
				auto tObj = dynamic_cast<const CGTeleport *>(obj);
				if(TeleportChannel::IMPASSABLE == ai->knownTeleportChannels[tObj->channel]->passability)
					break;
				for(auto exit : ai->knownTeleportChannels[tObj->channel]->exits)
				{
					if(!cb->getObj(exit))
					{ // Always attempt to visit two-way teleports if one of channel exits is not visible
						objs.push_back(obj);
						break;
					}
				}
				break;
			}
		}
	}

	auto primaryHero = ai->primaryHero().h;
	for(auto h : heroes)
	{
		for(auto obj : objs) //double loop, performance risk?
		{
			auto waysToVisitObj = ai->ah->howToVisitObj(h, obj);

			vstd::concatenate(ret, waysToVisitObj);
		}

		int3 t = whereToExplore(h);
		if(t.valid())
		{
			ret.push_back(sptr(VisitTile(t).sethero(h)));
		}
		else
		{
			//FIXME: possible issues when gathering army to break
			if(hero.h == h || //exporation is assigned to this hero
				(!hero && h == primaryHero)) //not assigned to specific hero, let our main hero do the job
			{
				t = ai->explorationDesperate(h);  //check this only ONCE, high cost
				if (t.valid()) //don't waste time if we are completely blocked
				{
					auto waysToVisitTile = ai->ah->howToVisitTile(h, t);

					vstd::concatenate(ret, waysToVisitTile);
					continue;
				}
			}
			ai->markHeroUnableToExplore(h); //there is no freely accessible tile, do not poll this hero anymore
		}
	}
	//we either don't have hero yet or none of heroes can explore
	if((!hero || ret.empty()) && ai->canRecruitAnyHero())
		ret.push_back(sptr(RecruitHero()));

	if(ret.empty())
	{
		throw goalFulfilledException(sptr(Explore().sethero(hero)));
	}
	//throw cannotFulfillGoalException("Cannot explore - no possible ways found!");

	return ret;
}

bool Explore::fulfillsMe(TSubgoal goal)
{
	if(goal->goalType == EXPLORE)
	{
		if(goal->hero)
			return hero == goal->hero;
		else
			return true; //cancel ALL exploration
	}
	return false;
}
