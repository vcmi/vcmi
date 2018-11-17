/*
* GatherArmy.cpp, part of VCMI engine
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

bool GatherArmy::operator==(const GatherArmy & other) const
{
	return other.hero.h == hero.h || town == other.town;
}

std::string GatherArmy::completeMessage() const
{
	return "Hero " + hero.get()->name + " gathered army of value " + boost::lexical_cast<std::string>(value);
}

TSubgoal GatherArmy::whatToDoToAchieve()
{
	//TODO: find hero if none set
	assert(hero.h);

	return fh->chooseSolution(getAllPossibleSubgoals()); //find dwelling. use current hero to prevent him from doing nothing.
}

static const BuildingID unitsSource[] = { BuildingID::DWELL_LVL_1, BuildingID::DWELL_LVL_2, BuildingID::DWELL_LVL_3,
	BuildingID::DWELL_LVL_4, BuildingID::DWELL_LVL_5, BuildingID::DWELL_LVL_6, BuildingID::DWELL_LVL_7};

TGoalVec GatherArmy::getAllPossibleSubgoals()
{
	//get all possible towns, heroes and dwellings we may use
	TGoalVec ret;

	if(!hero.validAndSet())
	{
		return ret;
	}

	//TODO: include evaluation of monsters gather in calculation
	for(auto t : cb->getTownsInfo())
	{
		auto pos = t->visitablePos();
		if(ai->isAccessibleForHero(pos, hero))
		{
			//grab army from town
			if(!t->visitingHero && howManyReinforcementsCanGet(hero, t))
			{
				if(!vstd::contains(ai->townVisitsThisWeek[hero], t))
					ret.push_back(sptr(VisitTile(pos).sethero(hero)));
			}
			//buy army in town
			if (!t->visitingHero || t->visitingHero == hero.get(true))
			{
				ui32 val = std::min<ui32>(value, howManyReinforcementsCanBuy(hero, t));
				if (val)
				{
					auto goal = sptr(BuyArmy(t, val).sethero(hero));
					if(!ai->ah->containsObjective(goal)) //avoid loops caused by reserving same objective twice
						ret.push_back(goal);
					else
						logAi->debug("Can not buy army, because of ai->ah->containsObjective");
				}
			}
			//build dwelling
			//TODO: plan building over multiple turns?
			//auto bid = ah->canBuildAnyStructure(t, std::vector<BuildingID>(unitsSource, unitsSource + ARRAY_COUNT(unitsSource)), 8 - cb->getDate(Date::DAY_OF_WEEK));

			//Do not use below code for now, rely on generic Build. Code below needs to know a lot of town/resource context to do more good than harm
			/*auto bid = ai->ah->canBuildAnyStructure(t, std::vector<BuildingID>(unitsSource, unitsSource + ARRAY_COUNT(unitsSource)), 1);
			if (bid.is_initialized())
			{
				auto goal = sptr(BuildThis(bid.get(), t).setpriority(priority));
				if (!ai->ah->containsObjective(goal)) //avoid loops caused by reserving same objective twice
					ret.push_back(goal);
				else
					logAi->debug("Can not build a structure, because of ai->ah->containsObjective");
			}*/
		}
	}

	auto otherHeroes = cb->getHeroesInfo();
	auto heroDummy = hero;
	vstd::erase_if(otherHeroes, [heroDummy](const CGHeroInstance * h)
	{
		if(h == heroDummy.h)
			return true;
		else if(!ai->isAccessibleForHero(heroDummy->visitablePos(), h, true))
			return true;
		else if(!ai->canGetArmy(heroDummy.h, h)) //TODO: return actual aiValue
			return true;
		else if(ai->getGoal(h)->goalType == GATHER_ARMY)
			return true;
		else
		   return false;
	});
	for(auto h : otherHeroes)
	{
		// Go to the other hero if we are faster
		if (!vstd::contains(ai->visitedHeroes[hero], h)
			&& ai->isAccessibleForHero(h->visitablePos(), hero, true)) //visit only once each turn //FIXME: this is only bug workaround
			ret.push_back(sptr(VisitHero(h->id.getNum()).setisAbstract(true).sethero(hero)));
		// Let the other hero come to us
		if (!vstd::contains(ai->visitedHeroes[h], hero))
			ret.push_back(sptr(VisitHero(hero->id.getNum()).setisAbstract(true).sethero(h)));
	}

	std::vector<const CGObjectInstance *> objs;
	for(auto obj : ai->visitableObjs)
	{
		if(obj->ID == Obj::CREATURE_GENERATOR1)
		{
			auto relationToOwner = cb->getPlayerRelations(obj->getOwner(), ai->playerID);

			//Use flagged dwellings only when there are available creatures that we can afford
			if(relationToOwner == PlayerRelations::SAME_PLAYER)
			{
				auto dwelling = dynamic_cast<const CGDwelling *>(obj);

				ui32 val = std::min<ui32>(value, howManyReinforcementsCanBuy(hero, dwelling));

				if(val)
				{
					for(auto & creLevel : dwelling->creatures)
					{
						if(creLevel.first)
						{
							for(auto & creatureID : creLevel.second)
							{
								auto creature = VLC->creh->creatures[creatureID];
								if(ai->ah->freeResources().canAfford(creature->cost))
									objs.push_back(obj); //TODO: reserve resources?
							}
						}
					}
				}
			}
		}
	}
	for(auto h : cb->getHeroesInfo())
	{
		for(auto obj : objs)
		{
			//find safe dwelling
			if(ai->isGoodForVisit(obj, h))
			{
				vstd::concatenate(ret, ai->ah->howToVisitObj(h, obj));
			}
		}
	}

	if(ai->canRecruitAnyHero() && ai->ah->freeGold() > GameConstants::HERO_GOLD_COST) //this is not stupid in early phase of game
	{
		if(auto t = ai->findTownWithTavern())
		{
			for(auto h : cb->getAvailableHeroes(t)) //we assume that all towns have same set of heroes
			{
				if(h && h->getTotalStrength() > 500) //do not buy heroes with single creatures for GatherArmy
				{
					ret.push_back(sptr(RecruitHero()));
					break;
				}
			}
		}
	}

	if(ret.empty())
	{
		if(hero == ai->primaryHero())
			ret.push_back(sptr(Explore()));
		else
			throw cannotFulfillGoalException("No ways to gather army");
	}

	return ret;
}
