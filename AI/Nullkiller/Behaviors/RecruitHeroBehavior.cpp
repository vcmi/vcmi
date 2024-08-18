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
#include "../lib/CHeroHandler.h"

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
	// If we don't have any heros we might want to lower our expectations.
	if (ourHeroes.empty())
		minScoreToHireMain = 0;

	const CGHeroInstance* bestHeroToHire = nullptr;
	const CGTownInstance* bestTownToHireFrom = nullptr;
	float bestScore = 0;
	bool haveCapitol = false;

	ai->dangerHitMap->updateHitMap();
	
	for(auto town : towns)
	{
		uint8_t closestThreat = UINT8_MAX;
		for (auto threat : ai->dangerHitMap->getTownThreats(town))
		{
			closestThreat = std::min(closestThreat, threat.turn);
		}
		//Don' hire a hero in a threatened town as one would have to stay outside
		if (closestThreat <= 1 && (town->visitingHero || town->garrisonHero))
			continue;
		if(ai->heroManager->canRecruitHero(town))
		{
			auto availableHeroes = ai->cb->getAvailableHeroes(town);

			for(auto hero : availableHeroes)
			{
				auto score = ai->heroManager->evaluateHero(hero);
				if(score > minScoreToHireMain)
				{
					score *= score / minScoreToHireMain;
				}
				score *= hero->getArmyCost();
				if (hero->type->heroClass->faction == town->getFaction())
					score *= 1.5;
				score *= town->getTownLevel();
				if (score > bestScore)
				{
					bestScore = score;
					bestHeroToHire = hero;
					bestTownToHireFrom = town;
				}
			}
		}
		if (town->hasCapitol())
			haveCapitol = true;
	}
	if (bestHeroToHire && bestTownToHireFrom)
	{
		if (ai->cb->getHeroesInfo().size() < ai->cb->getTownsInfo().size() + 1
			|| (ai->getFreeResources()[EGameResID::GOLD] > 10000 && !ai->buildAnalyzer->isGoldPressureHigh() && haveCapitol))
		{
			tasks.push_back(Goals::sptr(Goals::RecruitHero(bestTownToHireFrom, bestHeroToHire).setpriority((float)3 / ourHeroes.size())));
		}
	}

	return tasks;
}

}
