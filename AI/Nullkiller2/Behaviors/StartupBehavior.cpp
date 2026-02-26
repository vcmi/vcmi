/*
* StartupBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "StartupBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/BuildThis.h"
#include "../Goals/RecruitHero.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "../../../lib/mapObjects/CGResource.h"
#include "../Engine/Nullkiller.h"

namespace NK2AI
{

using namespace Goals;

std::string StartupBehavior::toString() const
{
	return "Startup";
}

const AIPath getShortestPath(const CGTownInstance * town, const std::vector<AIPath> & paths)
{
	auto shortestPath = *vstd::minElementByFun(paths, [town](const AIPath & path) -> float
	{
		if(town->getGarrisonHero() && path.targetHero == town->getGarrisonHero())
			return 1;

		return path.movementCost();
	});

	return shortestPath;
}

const CGHeroInstance * getNearestHero(const Nullkiller * aiNk, const CGTownInstance * town)
{
	auto paths = aiNk->pathfinder->getPathInfo(town->visitablePos());

	if(paths.empty())
		return nullptr;

	auto shortestPath = getShortestPath(town, paths);

	if(shortestPath.nodes.size() > 1
		|| shortestPath.turn() != 0
		|| shortestPath.targetHero->visitablePos().dist2dSQ(town->visitablePos()) > 4
		|| (town->getGarrisonHero() && shortestPath.targetHero == town->getGarrisonHero()))
		return nullptr;

	return shortestPath.targetHero;
}

bool needToRecruitHero(const Nullkiller * aiNk, const CGTownInstance * startupTown)
{
	if(!aiNk->heroManager->canRecruitHero(startupTown))
		return false;

	if(!startupTown->getGarrisonHero() && !startupTown->getVisitingHero())
		return true;

	int treasureSourcesCount = 0;

	for(auto obj : aiNk->objectClusterizer->getNearbyObjects())
	{
		auto armed = dynamic_cast<const CArmedInstance *>(obj);

		if(armed && armed->getArmyStrength() > 0)
			continue;

		bool isGoldPile = dynamic_cast<const CGResource *>(obj)
			&& dynamic_cast<const CGResource *>(obj)->resourceID() == EGameResID::GOLD;

		auto rewardable = dynamic_cast<const Rewardable::Interface *>(obj);

		if(rewardable)
		{
			for(auto & info : rewardable->configuration.info)
				if(info.reward.resources[EGameResID::GOLD] > 0)
					isGoldPile = true;
		}

		if(isGoldPile
			|| obj->ID == Obj::TREASURE_CHEST
			|| obj->ID == Obj::CAMPFIRE
			|| obj->ID == Obj::WATER_WHEEL)
		{
			treasureSourcesCount++;
		}
	}

	const auto basicCount = aiNk->cc->getTownsInfo().size() + 2;
	const auto boost = std::min(
		(int)std::floor(std::pow(1 + aiNk->cc->getMapSize().x / 50, 2)),
		treasureSourcesCount / 2);

	logAi->trace("Treasure sources found %d", treasureSourcesCount);
	logAi->trace("Startup allows %d+%d heroes", basicCount, boost);

	return aiNk->cc->getHeroCount(aiNk->playerID, true) < basicCount + boost;
}

Goals::TGoalVec StartupBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	auto towns = aiNk->cc->getTownsInfo();

	if(!towns.size())
		return tasks;

	const CGTownInstance * startupTown = towns.front();

	if(towns.size() > 1)
	{
		startupTown = *vstd::maxElementByFun(towns, [aiNk](const CGTownInstance * town) -> float
		{
			if(town->getGarrisonHero())
				return aiNk->heroManager->evaluateHero(town->getGarrisonHero());

			auto closestHero = getNearestHero(aiNk, town);

			if(closestHero)
				return aiNk->heroManager->evaluateHero(closestHero);

			return 0;
		});
	}

	if(!startupTown->hasBuilt(BuildingID::TAVERN)
		&& aiNk->cc->canBuildStructure(startupTown, BuildingID::TAVERN) == EBuildingState::ALLOWED)
	{
		tasks.push_back(Goals::sptr(Goals::BuildThis(BuildingID::TAVERN, startupTown).setpriority(100)));

		return tasks;
	}

	bool canRecruitHero = needToRecruitHero(aiNk, startupTown);
	auto closestHero = getNearestHero(aiNk, startupTown);

	if(closestHero)
	{
		if(!startupTown->getVisitingHero())
		{
			if(aiNk->armyManager->howManyReinforcementsCanGet(startupTown->getUpperArmy(), startupTown->getUpperArmy(), closestHero, TerrainId::NONE) > 200)
			{
				auto paths = aiNk->pathfinder->getPathInfo(startupTown->visitablePos());

				if(paths.size())
				{
					auto path = getShortestPath(startupTown, paths);

					tasks.push_back(Goals::sptr(Goals::ExecuteHeroChain(path, startupTown).setpriority(100)));
				}
			}
		}
		else
		{
			auto visitingHero = startupTown->getVisitingHero();
			auto visitingHeroScore = aiNk->heroManager->evaluateHero(visitingHero);
				
			if(startupTown->getGarrisonHero())
			{
				auto garrisonHero = startupTown->getGarrisonHero();
				auto garrisonHeroScore = aiNk->heroManager->evaluateHero(garrisonHero);

				if(visitingHeroScore > garrisonHeroScore
					|| (aiNk->heroManager->getHeroRoleOrDefaultInefficient(garrisonHero) == HeroRole::SCOUT && aiNk->heroManager->getHeroRoleOrDefaultInefficient(visitingHero) == HeroRole::MAIN))
				{
					if(canRecruitHero || aiNk->armyManager->howManyReinforcementsCanGet(visitingHero, garrisonHero) > 200)
					{
						tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, visitingHero, HeroLockedReason::STARTUP).setpriority(100)));
					}
				}
				else if(aiNk->armyManager->howManyReinforcementsCanGet(garrisonHero, visitingHero) > 200)
				{
					tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, garrisonHero, HeroLockedReason::STARTUP).setpriority(100)));
				}
			}
			else if(canRecruitHero)
			{
				auto canPickTownArmy = startupTown->stacksCount() == 0
					|| aiNk->armyManager->howManyReinforcementsCanGet(startupTown->getVisitingHero(), startupTown) > 0;

				if(canPickTownArmy)
				{
					tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, visitingHero, HeroLockedReason::STARTUP).setpriority(100)));
				}
			}
		}
	}

	if(tasks.empty() && canRecruitHero && !startupTown->getVisitingHero())
	{
		tasks.push_back(Goals::sptr(Goals::RecruitHero(startupTown)));
	}

	if(tasks.empty() && !startupTown->getVisitingHero())
	{
		for(auto town : towns)
		{
			if(!town->getVisitingHero() && needToRecruitHero(aiNk, town))
			{
				tasks.push_back(Goals::sptr(Goals::RecruitHero(town)));

				break;
			}
		}
	}

	if(tasks.empty() && towns.size())
	{
		for(const CGTownInstance * town : towns)
		{
			if(town->getGarrisonHero()
				&& town->getGarrisonHero()->movementPointsRemaining()
				&& !town->getVisitingHero()
				&& aiNk->getHeroLockedReason(town->getGarrisonHero()) != HeroLockedReason::DEFENCE)
			{
				tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(town, nullptr).setpriority(MIN_PRIORITY)));
			}
		}
	}

	return tasks;
}

}
