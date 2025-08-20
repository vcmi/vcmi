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

namespace NK2AI
{

using namespace Goals;

std::string RecruitHeroBehavior::toString() const
{
	return "Recruit hero";
}

Goals::TGoalVec RecruitHeroBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	const auto ourTowns = aiNk->cc->getTownsInfo();
	const auto ourHeroes = aiNk->heroManager->getHeroRoles();
	auto minScoreToHireMain = std::numeric_limits<float>::max();
	int currentArmyValue = 0;

	for(const auto & hero : ourHeroes)
	{
		currentArmyValue += hero.first->getArmyCost();
		if(hero.second != HeroRole::MAIN)
			continue;

		const auto newScore = aiNk->heroManager->evaluateHero(hero.first.get());

		if(minScoreToHireMain > newScore)
		{
			// weakest main hero score
			minScoreToHireMain = newScore;
		}
	}

	// If we don't have any heroes, lower our expectations.
	if (ourHeroes.empty())
		minScoreToHireMain = 0;

	const CGHeroInstance* bestHeroToHire = nullptr;
	const CGTownInstance* bestTownToHireFrom = nullptr;
	float bestScore = 0;
	bool haveCapitol = false;

	// Simplification: Moved this call before getting into the decomposer
	// aiNk->dangerHitMap->updateHitMap();

	int treasureSourcesCount = 0;
	int bestClosestThreat = UINT8_MAX;
	
	for(const auto * town : ourTowns)
	{
		uint8_t closestThreat = UINT8_MAX;
		for (const auto & threat : aiNk->dangerHitMap->getTownThreats(town))
		{
			closestThreat = std::min(closestThreat, threat.turn);
		}

		if (town->getVisitingHero() && town->getGarrisonHero())
			continue;

		float visitability = 0;
		for (const auto & hero : ourHeroes)
		{
			if (aiNk->dangerHitMap->getClosestTown(hero.first.get()->visitablePos()) == town)
				visitability++;
		}

		if(aiNk->heroManager->canRecruitHero(town))
		{
			auto availableHeroes = aiNk->cc->getAvailableHeroes(town);
			
			for (const auto * obj : aiNk->objectClusterizer->getNearbyObjects())
			{
				if (obj->ID == Obj::RESOURCE
					|| obj->ID == Obj::TREASURE_CHEST
					|| obj->ID == Obj::CAMPFIRE
					|| isWeeklyRevisitable(aiNk->playerID, obj)
					|| obj->ID == Obj::ARTIFACT)
				{
					auto tile = obj->visitablePos();
					if (town == aiNk->dangerHitMap->getClosestTown(tile))
						treasureSourcesCount++; // TODO: Mircea: Shouldn't it be used to determine the best town?
				}
			}

			for(const auto hero : availableHeroes)
			{
				if ((town->getVisitingHero() || town->getGarrisonHero()) 
					&& closestThreat < 1
					&& hero->getArmyCost() < GameConstants::HERO_GOLD_COST / 3.0)
					continue;

				auto score = aiNk->heroManager->evaluateHero(hero);
				if(score > minScoreToHireMain)
				{
					score *= score / minScoreToHireMain;
				}
				score *= hero->getArmyCost() + currentArmyValue;

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
					bestTownToHireFrom = town; // TODO: Mircea: Seems to be no logic to choose the right town?
					bestClosestThreat = closestThreat;
				}
			}
		}

		if (town->hasCapitol())
			haveCapitol = true;
	}

	if (bestHeroToHire && bestTownToHireFrom)
	{
		if (aiNk->cc->getHeroesInfo().size() == 0
			|| treasureSourcesCount > aiNk->cc->getHeroesInfo().size() * 5
			|| (bestHeroToHire->getArmyCost() > GameConstants::HERO_GOLD_COST / 2.0 && (bestClosestThreat < 1 || !aiNk->buildAnalyzer->isGoldPressureOverMax()))
			|| (aiNk->getFreeResources()[EGameResID::GOLD] > 10000 && !aiNk->buildAnalyzer->isGoldPressureOverMax() && haveCapitol)
			|| (aiNk->getFreeResources()[EGameResID::GOLD] > 30000 && !aiNk->buildAnalyzer->isGoldPressureOverMax()))
		{
			tasks.push_back(Goals::sptr(Goals::RecruitHero(bestTownToHireFrom, bestHeroToHire).setpriority((float)3 / (ourHeroes.size() + 1))));
		}
	}

	return tasks;
}

}
