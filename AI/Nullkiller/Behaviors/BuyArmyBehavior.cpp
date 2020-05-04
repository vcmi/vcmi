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
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Engine/Nullkiller.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

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

	if(ai->nullkiller->buildAnalyzer->getGoldPreasure() > MAX_GOLD_PEASURE)
		return tasks;
		
	auto heroes = cb->getHeroesInfo();

	if(heroes.size())
	{
		auto mainArmy = vstd::maxElementByFun(heroes, [](const CGHeroInstance * hero) -> uint64_t
		{
			return hero->getTotalStrength();
		});

		for(auto town : cb->getTownsInfo())
		{
			const CGHeroInstance * targetHero = *mainArmy;

			/*if(town->visitingHero)
			{
				targetHero = town->visitingHero.get();

				if(ai->nullkiller->armyManager->howManyReinforcementsCanGet(targetHero, town->getUpperArmy()))
				{
					tasks.push_back(sptr(VisitTile(town->visitablePos()).sethero(targetHero).setpriority(5)));

					continue;
				}
			}*/

			auto reinforcement = ai->nullkiller->armyManager->howManyReinforcementsCanBuy(targetHero, town);

			if(reinforcement)
				reinforcement = ai->nullkiller->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town);

			if(reinforcement)
			{
				tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(5)));
			}
		}
	}

	return tasks;
}
