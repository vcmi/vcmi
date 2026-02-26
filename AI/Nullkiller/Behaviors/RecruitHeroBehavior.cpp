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
	int currentArmyValue = 0;

	for(auto hero : ourHeroes)
	{
		currentArmyValue += hero.first->getArmyCost();
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
	int treasureSourcesCount = 0;
	int bestClosestThreat = UINT8_MAX;
	
	for(auto town : towns)
	{
		uint8_t closestThreat = UINT8_MAX;
		for (auto threat : ai->dangerHitMap->getTownThreats(town))
		{
			closestThreat = std::min(closestThreat, threat.turn);
		}
		//Don't hire a hero where there already is one present
		if (town->getVisitingHero() && town->getGarrisonHero())
			continue;
		float visitability = 0;
		for (auto checkHero : ourHeroes)
		{
			if (ai->dangerHitMap->getClosestTown(checkHero.first.get()->visitablePos()) == town)
				visitability++;
		}
		if(ai->heroManager->canRecruitHero(town))
		{
			auto availableHeroes = ai->cb->getAvailableHeroes(town);
			
			for (auto obj : ai->objectClusterizer->getNearbyObjects())
			{
				if ((obj->ID == Obj::RESOURCE)
					|| obj->ID == Obj::TREASURE_CHEST
					|| obj->ID == Obj::CAMPFIRE
					|| isWeeklyRevisitable(ai, obj)
					|| obj->ID == Obj::ARTIFACT)
				{
					auto tile = obj->visitablePos();
					auto closestTown = ai->dangerHitMap->getClosestTown(tile);

					if (town == closestTown)
						treasureSourcesCount++;
				}
			}

			for(auto hero : availableHeroes)
			{
				if ((town->getVisitingHero() || town->getGarrisonHero()) 
					&& closestThreat < 1
					&& hero->getArmyCost() < GameConstants::HERO_GOLD_COST / 3.0)
					continue;
				auto score = ai->heroManager->evaluateHero(hero);
				if(score > minScoreToHireMain)
				{
					score *= score / minScoreToHireMain;
				}
				score *= (hero->getArmyCost() + currentArmyValue);
				if (hero->getFactionID() == town->getFactionID())
					score *= 1.5;
				if (vstd::isAlmostZero(visitability))
					score *= 30 * town->getTownLevel();
				else
					score *= town->getTownLevel() / visitability;
				if (score > bestScore)
				{
					bestScore = score;
					bestHeroToHire = hero;
					bestTownToHireFrom = town;
					bestClosestThreat = closestThreat;
				}
			}
		}
		if (town->hasCapitol())
			haveCapitol = true;
	}
	if (bestHeroToHire && bestTownToHireFrom)
	{
		if (ai->cb->getHeroesInfo().size() == 0
			|| treasureSourcesCount > ai->cb->getHeroesInfo().size() * 5
			|| (bestHeroToHire->getArmyCost() > GameConstants::HERO_GOLD_COST / 2.0 && (bestClosestThreat < 1 || !ai->buildAnalyzer->isGoldPressureHigh()))
			|| (ai->getFreeResources()[EGameResID::GOLD] > 10000 && !ai->buildAnalyzer->isGoldPressureHigh() && haveCapitol)
			|| (ai->getFreeResources()[EGameResID::GOLD] > 30000 && !ai->buildAnalyzer->isGoldPressureHigh()))
		{
			tasks.push_back(Goals::sptr(Goals::RecruitHero(bestTownToHireFrom, bestHeroToHire).setpriority((float)3 / (ourHeroes.size() + 1))));
		}
	}

	return tasks;
}

}
