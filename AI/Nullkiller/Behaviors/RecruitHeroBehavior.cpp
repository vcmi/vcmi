/*
* RecruitHeroBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "RecruitHeroBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/RecruitHero.h"
#include "../Goals/ExecuteHeroChain.h"

namespace NKAI
{

using namespace Goals;

std::string RecruitHeroBehavior::toString() const
{
	return "Recruit hero";
}

Goals::TGoalVec RecruitHeroBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;
	auto towns = ai->cb->getTownsInfo();

	auto ourHeroes = ai->heroManager->getHeroRoles();
	auto minScoreToHireMain = std::numeric_limits<float>::max();

	for(auto hero : ourHeroes)
	{
		if(hero.second != HeroRole::MAIN)
			continue;

		auto newScore = ai->heroManager->evaluateHero(hero.first.get());

		if(minScoreToHireMain > newScore)
		{
			// weakest main hero score
			minScoreToHireMain = newScore;
		}
	}

	for(auto town : towns)
	{
		if(ai->heroManager->canRecruitHero(town))
		{
			auto availableHeroes = ai->cb->getAvailableHeroes(town);

			for(auto hero : availableHeroes)
			{
				auto score = ai->heroManager->evaluateHero(hero);

				if(score > minScoreToHireMain)
				{
					tasks.push_back(Goals::sptr(Goals::RecruitHero(town, hero).setpriority(200)));
					break;
				}
			}

			int treasureSourcesCount = 0;

			for(auto obj : ai->objectClusterizer->getNearbyObjects())
			{
				if((obj->ID == Obj::RESOURCE)
					|| obj->ID == Obj::TREASURE_CHEST
					|| obj->ID == Obj::CAMPFIRE
					|| isWeeklyRevisitable(ai, obj)
					|| obj->ID ==Obj::ARTIFACT)
				{
					auto tile = obj->visitablePos();
					auto closestTown = ai->dangerHitMap->getClosestTown(tile);

					if(town == closestTown)
						treasureSourcesCount++;
				}
			}

			if(treasureSourcesCount < 5 && (town->garrisonHero || town->getUpperArmy()->getArmyStrength() < 10000))
				continue;

			if(ai->cb->getHeroesInfo().size() < ai->cb->getTownsInfo().size() + 1
				|| (ai->getFreeResources()[EGameResID::GOLD] > 10000 && !ai->buildAnalyzer->isGoldPressureHigh()))
			{
				tasks.push_back(Goals::sptr(Goals::RecruitHero(town).setpriority(3)));
			}
		}
	}

	return tasks;
}

}
