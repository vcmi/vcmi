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
#include "../Goals/Composition.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/RecruitHero.h"
#include <algorithm>

namespace NK2AI
{

using namespace Goals;

static constexpr float DEFENSIVE_EMERGENCY_RECRUIT_PRIORITY = 1000000.0f;

std::string RecruitHeroBehavior::toString() const
{
	return "Recruit hero";
}

Goals::TGoalVec RecruitHeroBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	const auto ourTowns = aiNk->cc->getTownsInfo();
	const auto ourHeroes = aiNk->cc->getHeroesInfo();
	constexpr RecruitHeroChoice bestChoice;
	bool haveCapitol = false;
	int treasureSourcesCount = 0;

	// Simplification: Moved this call before getting into the decomposer
	// aiNk->dangerHitMap->updateHitMap();

	for(const auto * town : ourTowns)
	{
		// TODO: Mircea: Should consider removing it if necessary
		// What if under threat and there is a stronger hero available?

		if(town->getVisitingHero() && town->getGarrisonHero())
			continue;

		const auto & townThreats = aiNk->dangerHitMap->getTownThreats(town);

		float visitabilityRatio = 0;
		for(const auto * const hero : ourHeroes)
		{
			if(aiNk->dangerHitMap->getClosestTown(hero->visitablePos()) == town)
				visitabilityRatio += 1.0f / ourHeroes.size();
		}

		// TODO: Mircea: Should check even if hero cap is reached because it should replace heroes if a stronger one appears
		if(aiNk->heroManager->canRecruitHero(town))
		{
			calculateTreasureSources(aiNk->objectClusterizer->getNearbyObjects(), aiNk->playerID, *aiNk->dangerHitMap, treasureSourcesCount, *town);
			calculateBestHero(
				aiNk->cc->getAvailableHeroes(town),
				*aiNk->heroManager,
				bestChoice,
				*town,
				townThreats,
				aiNk->settings->getSafeAttackRatio(),
				visitabilityRatio);
		}

		if(town->hasCapitol())
			haveCapitol = true;
	}

	calculateFinalDecision(*aiNk, tasks, ourHeroes, bestChoice, haveCapitol, treasureSourcesCount);
	return tasks;
}

void RecruitHeroBehavior::calculateTreasureSources(
	const std::vector<const CGObjectInstance *> & nearbyObjects,
	const PlayerColor & playerID,
	const DangerHitMapAnalyzer & dangerHitMap,
	int & treasureSourcesCount,
	const CGTownInstance & town
)
{
	for(const auto * obj : nearbyObjects)
	{
		if(obj->ID == Obj::RESOURCE || obj->ID == Obj::TREASURE_CHEST || obj->ID == Obj::CAMPFIRE || isWeeklyRevisitable(playerID, obj)
		   || obj->ID == Obj::ARTIFACT)
		{
			if(&town == dangerHitMap.getClosestTown(obj->visitablePos()))
				treasureSourcesCount++; // TODO: Mircea: Shouldn't it be used to determine the best town?
		}
	}
}

void RecruitHeroBehavior::calculateBestHero(
	const std::vector<const CGHeroInstance *> & availableHeroes,
	const HeroManager & heroManager,
	const RecruitHeroChoice & bestChoice,
	const CGTownInstance & town,
	const std::vector<HitMapInfo> & townThreats,
	const float safeAttackRatio,
	const float visitabilityRatio
)
{
	uint8_t closestThreatTurn = UINT8_MAX;
	for(const auto & threat : townThreats)
		closestThreatTurn = std::min(closestThreatTurn, threat.turn);

	for(const auto * const hero : availableHeroes)
	{
		if((town.getVisitingHero() || town.getGarrisonHero()) && closestThreatTurn < 1 && hero->getArmyCost() < GameConstants::HERO_GOLD_COST / 3.0)
			continue;

		const float heroScore = heroManager.evaluateHero(hero);
		float totalScore = heroScore + hero->getArmyCost();

		// TODO: Mircea: Score higher if ballista/tent/ammo cart by the cost in gold? Or should that be covered in evaluateHero?
		// getArtifactScoreForHero(hero, ...) ArtifactID::BALLISTA

		if(hero->getFactionID() == town.getFactionID())
			totalScore += heroScore * 1.5;

		// prioritize a more developed town especially if no heroes can visit it (smaller ratio, bigger score)
		totalScore += heroScore * town.getTownLevel() * (1 - visitabilityRatio);

		bool defensiveEmergency = false;
		for(const auto & threat : townThreats)
		{
			if(isDefensiveRecruitEmergency(town, *hero, threat, safeAttackRatio))
			{
				defensiveEmergency = true;
				break;
			}
		}

		if((defensiveEmergency && !bestChoice.defensiveEmergency)
		   || (defensiveEmergency == bestChoice.defensiveEmergency && totalScore > bestChoice.score))
		{
			bestChoice.score = totalScore;
			bestChoice.hero = hero;
			bestChoice.town = &town;
			bestChoice.defensiveEmergency = defensiveEmergency;
		}
	}
}

void RecruitHeroBehavior::calculateFinalDecision(
	const Nullkiller & aiNk,
	Goals::TGoalVec & tasks,
	const std::vector<const CGHeroInstance *> & ourHeroes,
	const RecruitHeroChoice & bestChoice,
	const bool haveCapitol,
	const int treasureSourcesCount
)
{
	if(bestChoice.hero != nullptr && !vstd::isAlmostZero(bestChoice.score))
	{
		if(shouldRecruitHero(
			   ourHeroes.size(),
			   bestChoice,
			   haveCapitol,
			   treasureSourcesCount,
			   aiNk.getFreeResources()[EGameResID::GOLD],
			   aiNk.buildAnalyzer->isGoldPressureOverMax()))
		{
			if(bestChoice.defensiveEmergency)
			{
				TGoalVec sequence;
				sequence.push_back(sptr(Goals::RecruitHero(bestChoice.town, bestChoice.hero)));
				sequence.push_back(sptr(Goals::ExchangeSwapTownHeroes(bestChoice.town, bestChoice.hero, HeroLockedReason::DEFENCE)));

				Goals::Composition composition;
				composition.addNextSequence(sequence);
				composition.setpriority(DEFENSIVE_EMERGENCY_RECRUIT_PRIORITY);
				tasks.push_back(Goals::sptr(composition));
			}
			else
				tasks.push_back(Goals::sptr(Goals::RecruitHero(bestChoice.town, bestChoice.hero).setpriority(3.0 / (ourHeroes.size() + 1))));
		}
	}
}

}
