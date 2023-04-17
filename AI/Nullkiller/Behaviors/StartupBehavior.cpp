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
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/mapObjects/MapObjects.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Engine/Nullkiller.h"

namespace NKAI
{

extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

std::string StartupBehavior::toString() const
{
	return "Startup";
}

const AIPath getShortestPath(const CGTownInstance * town, const std::vector<AIPath> & paths)
{
	auto shortestPath = *vstd::minElementByFun(paths, [town](const AIPath & path) -> float
	{
		if(town->garrisonHero && path.targetHero == town->garrisonHero.get())
			return 1;

		return path.movementCost();
	});

	return shortestPath;
}

const CGHeroInstance * getNearestHero(const CGTownInstance * town)
{
	auto paths = ai->nullkiller->pathfinder->getPathInfo(town->visitablePos());

	if(paths.empty())
		return nullptr;

	auto shortestPath = getShortestPath(town, paths);

	if(shortestPath.nodes.size() > 1
		|| shortestPath.turn() != 0
		|| shortestPath.targetHero->visitablePos().dist2dSQ(town->visitablePos()) > 4
		|| (town->garrisonHero && shortestPath.targetHero == town->garrisonHero.get()))
		return nullptr;

	return shortestPath.targetHero;
}

bool needToRecruitHero(const CGTownInstance * startupTown)
{
	if(!ai->canRecruitAnyHero(startupTown))
		return false;

	if(!startupTown->garrisonHero && !startupTown->visitingHero)
		return true;

	int treasureSourcesCount = 0;

	for(auto obj : ai->nullkiller->objectClusterizer->getNearbyObjects())
	{
		if((obj->ID == Obj::RESOURCE && obj->subID == GameResID(EGameResID::GOLD))
			|| obj->ID == Obj::TREASURE_CHEST
			|| obj->ID == Obj::CAMPFIRE
			|| obj->ID == Obj::WATER_WHEEL)
		{
			treasureSourcesCount++;
		}
	}

	auto basicCount = cb->getTownsInfo().size() + 2;
	auto boost = std::min(
		(int)std::floor(std::pow(1 + (cb->getMapSize().x / 50), 2)),
		treasureSourcesCount / 2);

	logAi->trace("Treasure sources found %d", treasureSourcesCount);
	logAi->trace("Startup allows %d+%d heroes", basicCount, boost);

	return cb->getHeroCount(ai->playerID, true) < basicCount + boost;
}

Goals::TGoalVec StartupBehavior::decompose() const
{
	Goals::TGoalVec tasks;
	auto towns = cb->getTownsInfo();

	if(!towns.size())
		return tasks;

	const CGTownInstance * startupTown = towns.front();

	if(towns.size() > 1)
	{
		startupTown = *vstd::maxElementByFun(towns, [](const CGTownInstance * town) -> float
		{
			if(town->garrisonHero)
				return ai->nullkiller->heroManager->evaluateHero(town->garrisonHero.get());

			auto closestHero = getNearestHero(town);

			if(closestHero)
				return ai->nullkiller->heroManager->evaluateHero(closestHero);

			return 0;
		});
	}

	if(!startupTown->hasBuilt(BuildingID::TAVERN)
		&& cb->canBuildStructure(startupTown, BuildingID::TAVERN) == EBuildingState::ALLOWED)
	{
		tasks.push_back(Goals::sptr(Goals::BuildThis(BuildingID::TAVERN, startupTown).setpriority(100)));

		return tasks;
	}

	bool canRecruitHero = needToRecruitHero(startupTown);
	auto closestHero = getNearestHero(startupTown);

	if(closestHero)
	{
		if(!startupTown->visitingHero)
		{
			if(ai->nullkiller->armyManager->howManyReinforcementsCanGet(startupTown->getUpperArmy(), startupTown->getUpperArmy(), closestHero) > 200)
			{
				auto paths = ai->nullkiller->pathfinder->getPathInfo(startupTown->visitablePos());

				if(paths.size())
				{
					auto path = getShortestPath(startupTown, paths);

					tasks.push_back(Goals::sptr(Goals::ExecuteHeroChain(path, startupTown).setpriority(100)));
				}
			}
		}
		else
		{
			auto visitingHero = startupTown->visitingHero.get();
			auto visitingHeroScore = ai->nullkiller->heroManager->evaluateHero(visitingHero);
				
			if(startupTown->garrisonHero)
			{
				auto garrisonHero = startupTown->garrisonHero.get();
				auto garrisonHeroScore = ai->nullkiller->heroManager->evaluateHero(garrisonHero);

				if(visitingHeroScore > garrisonHeroScore
					|| (ai->nullkiller->heroManager->getHeroRole(garrisonHero) == HeroRole::SCOUT && ai->nullkiller->heroManager->getHeroRole(visitingHero) == HeroRole::MAIN))
				{
					if(canRecruitHero || ai->nullkiller->armyManager->howManyReinforcementsCanGet(visitingHero, garrisonHero) > 200)
					{
						tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, visitingHero, HeroLockedReason::STARTUP).setpriority(100)));
					}
				}
				else if(ai->nullkiller->armyManager->howManyReinforcementsCanGet(garrisonHero, visitingHero) > 200)
				{
					tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, garrisonHero, HeroLockedReason::STARTUP).setpriority(100)));
				}
			}
			else if(canRecruitHero)
			{
				auto canPickTownArmy = startupTown->stacksCount() == 0
					|| ai->nullkiller->armyManager->howManyReinforcementsCanGet(startupTown->visitingHero, startupTown) > 0;

				if(canPickTownArmy)
				{
					tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, visitingHero, HeroLockedReason::STARTUP).setpriority(100)));
				}
			}
		}
	}

	if(tasks.empty() && canRecruitHero && !startupTown->visitingHero)
	{
		tasks.push_back(Goals::sptr(Goals::RecruitHero(startupTown)));
	}

	if(tasks.empty() && !startupTown->visitingHero)
	{
		for(auto town : towns)
		{
			if(!town->visitingHero && needToRecruitHero(town))
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
			if(town->garrisonHero
				&& town->garrisonHero->movement
				&& !town->visitingHero
				&& ai->nullkiller->getHeroLockedReason(town->garrisonHero) != HeroLockedReason::DEFENCE)
			{
				tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(town, nullptr).setpriority(MIN_PRIORITY)));
			}
		}
	}

	return tasks;
}

}
