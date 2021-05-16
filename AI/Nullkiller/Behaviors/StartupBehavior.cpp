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
#include "../VCAI.h"
#include "../AIhelper.h"
#include "../AIUtility.h"
#include "../Goals/RecruitHero.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/mapObjects/MapObjects.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Engine/Nullkiller.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

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
	auto paths = ai->ah->getPathsToTile(town->visitablePos());

	if(paths.empty())
		return nullptr;

	auto shortestPath = getShortestPath(town, paths);

	if(shortestPath.nodes.size() > 1
		|| shortestPath.turn() != 0
		|| shortestPath.targetHero->visitablePos().dist2dSQ(town->visitablePos()) > 4
		|| town->garrisonHero && shortestPath.targetHero == town->garrisonHero.get())
		return nullptr;

	return shortestPath.targetHero;
}

bool needToRecruitHero(const CGTownInstance * startupTown)
{
	if(!ai->canRecruitAnyHero(startupTown))
		return false;

	if(!startupTown->garrisonHero && !startupTown->visitingHero)
		return false;

	auto heroToCheck = startupTown->garrisonHero ? startupTown->garrisonHero.get() : startupTown->visitingHero.get();
	auto paths = cb->getPathsInfo(heroToCheck);

	int treasureSourcesCount = 0;

	for(auto obj : ai->nullkiller->objectClusterizer->getNearbyObjects())
	{
		if(obj->ID == Obj::RESOURCE && obj->subID == Res::GOLD
			|| obj->ID == Obj::TREASURE_CHEST
			|| obj->ID == Obj::CAMPFIRE
			|| obj->ID == Obj::WATER_WHEEL)
		{
			auto path = paths->getPathInfo(obj->visitablePos());
			if((path->accessible == CGPathNode::BLOCKVIS || path->accessible == CGPathNode::VISIT) 
				&& path->reachable())
			{
				treasureSourcesCount++;
			}
		}
	}

	auto basicCount = cb->getTownsInfo().size() + 2;
	auto boost = (int)std::floor(std::pow(treasureSourcesCount / 3.0, 2));

	return cb->getHeroCount(ai->playerID, true) < basicCount + boost;
}

Goals::TGoalVec StartupBehavior::decompose() const
{
	Goals::TGoalVec tasks;
	auto towns = cb->getTownsInfo();

	if(!towns.size())
		return tasks;

	const CGTownInstance * startupTown = towns.front();
	bool canRecruitHero = needToRecruitHero(startupTown);

	if(towns.size() > 1)
	{
		startupTown = *vstd::maxElementByFun(towns, [](const CGTownInstance * town) -> float
		{
			auto closestHero = getNearestHero(town);

			if(!closestHero)
				return 0;

			return ai->ah->evaluateHero(closestHero);
		});
	}

	auto closestHero = getNearestHero(startupTown);

	if(closestHero)
	{
		if(!startupTown->visitingHero)
		{
			if(ai->ah->howManyReinforcementsCanGet(startupTown->getUpperArmy(), closestHero) > 200)
			{
				auto paths = ai->ah->getPathsToTile(startupTown->visitablePos());

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
			auto visitingHeroScore = ai->ah->evaluateHero(visitingHero);
				
			if(startupTown->garrisonHero)
			{
				auto garrisonHero = startupTown->garrisonHero.get();
				auto garrisonHeroScore = ai->ah->evaluateHero(garrisonHero);

				if(visitingHeroScore > garrisonHeroScore
					|| ai->ah->getHeroRole(garrisonHero) == HeroRole::SCOUT && ai->ah->getHeroRole(visitingHero) == HeroRole::MAIN)
				{
					if(canRecruitHero || ai->ah->howManyReinforcementsCanGet(visitingHero, garrisonHero) > 200)
					{
						tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, visitingHero, HeroLockedReason::STARTUP).setpriority(100)));
					}
				}
				else if(ai->ah->howManyReinforcementsCanGet(garrisonHero, visitingHero) > 200)
				{
					tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, garrisonHero, HeroLockedReason::STARTUP).setpriority(100)));
				}
			}
			else if(canRecruitHero)
			{
				tasks.push_back(Goals::sptr(ExchangeSwapTownHeroes(startupTown, visitingHero, HeroLockedReason::STARTUP).setpriority(100)));
			}
		}
	}

	if(tasks.empty() && canRecruitHero && !startupTown->visitingHero)
	{
		tasks.push_back(Goals::sptr(Goals::RecruitHero(startupTown)));
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
