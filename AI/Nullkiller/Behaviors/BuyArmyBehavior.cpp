/*
* BuyArmyBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "BuyArmyBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Engine/Nullkiller.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

std::string BuyArmyBehavior::toString() const
{
	return "Buy army";
}

Goals::TGoalVec BuyArmyBehavior::decompose() const
{
	Goals::TGoalVec tasks;

	if(cb->getDate(Date::DAY) == 1)
		return tasks;
		
	auto heroes = cb->getHeroesInfo();

	if(heroes.empty())
	{
		return tasks;
	}

	for(auto town : cb->getTownsInfo())
	{
		auto townArmyAvailableToBuy = ai->nullkiller->armyManager->getArmyAvailableToBuyAsCCreatureSet(
			town,
			ai->nullkiller->getFreeResources());

		for(const CGHeroInstance * targetHero : heroes)
		{
			if(ai->nullkiller->buildAnalyzer->getGoldPreasure() > MAX_GOLD_PEASURE
				&& !town->hasBuilt(BuildingID::CITY_HALL))
			{
				continue;
			}

			if(ai->nullkiller->heroManager->getHeroRole(targetHero) == HeroRole::MAIN)
			{
				auto reinforcement = ai->nullkiller->armyManager->howManyReinforcementsCanGet(
					targetHero,
					targetHero,
					&*townArmyAvailableToBuy);

				if(reinforcement)
					vstd::amin(reinforcement, ai->nullkiller->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town));

				if(reinforcement)
				{
					tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(5)));
				}
			}
		}
	}

	return tasks;
}

}
