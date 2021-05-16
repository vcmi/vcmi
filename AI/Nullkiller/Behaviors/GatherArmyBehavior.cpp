/*
* GatherArmyBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "../VCAI.h"
#include "../Engine/Nullkiller.h"
#include "../AIhelper.h"
#include "../Goals/ExecuteHeroChain.h"
#include "GatherArmyBehavior.h"
#include "../AIUtility.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

#define AI_TRACE_LEVEL 2

std::string GatherArmyBehavior::toString() const
{
	return "Gather army";
}

Goals::TGoalVec GatherArmyBehavior::getTasks()
{
	Goals::TGoalVec tasks;

	auto heroes = cb->getHeroesInfo();

	if(heroes.empty())
	{
		return tasks;
	}
	
	for(const CGHeroInstance * hero : heroes)
	{
		if(ai->ah->getHeroRole(hero) != HeroRole::MAIN
			|| hero->getArmyStrength() < 300)
		{
#ifdef AI_TRACE_LEVEL >= 1
			logAi->trace("Skipping hero %s", hero->name);
#endif
			continue;
		}

		const int3 pos = hero->visitablePos();

#ifdef AI_TRACE_LEVEL >= 1
		logAi->trace("Checking ways to gaher army for hero %s, %s", hero->getObjectName(), pos.toString());
#endif
		if(ai->nullkiller->isHeroLocked(hero))
		{
#ifdef AI_TRACE_LEVEL >= 1
			logAi->trace("Skipping locked hero %s, %s", hero->getObjectName(), pos.toString());
#endif
			continue;
		}

		auto paths = ai->ah->getPathsToTile(pos);
		std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;

#ifdef AI_TRACE_LEVEL >= 1
		logAi->trace("Found %d paths", paths.size());
#endif

		for(auto & path : paths)
		{
#ifdef AI_TRACE_LEVEL >= 2
			logAi->trace("Path found %s", path.toString());
#endif
			bool skip = path.targetHero == hero;

			for(auto node : path.nodes)
			{
				skip |= (node.targetHero == hero);
			}

			if(skip) continue;

#ifdef AI_TRACE_LEVEL >= 2
			logAi->trace("Path found %s", path.toString());
#endif

			if(path.getFirstBlockedAction())
			{
#ifdef AI_TRACE_LEVEL >= 2
				// TODO: decomposition?
				logAi->trace("Ignore path. Action is blocked.");
#endif
				continue;
			}

			if(ai->nullkiller->dangerHitMap->enemyCanKillOurHeroesAlongThePath(path))
			{
#ifdef AI_TRACE_LEVEL >= 2
				logAi->trace("Ignore path. Target hero can be killed by enemy. Our power %lld", path.heroArmy->getArmyStrength());
#endif
				continue;
			}

			float armyValue = (float)ai->ah->howManyReinforcementsCanGet(hero, path.heroArmy) / hero->getArmyStrength();

			// avoid transferring very small amount of army
			if(armyValue < 0.1f)
				continue;

			// avoid trying to move bigger army to the weaker one.
			if(armyValue > 0.5f)
				continue;

			auto danger = path.getTotalDanger();

			auto isSafe = isSafeToVisit(hero, path.heroArmy, danger);

#ifdef AI_TRACE_LEVEL >= 2
			logAi->trace(
				"It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld",
				isSafe ? "safe" : "not safe",
				hero->name,
				path.targetHero->name,
				path.getHeroStrength(),
				danger,
				path.getTotalArmyLoss());
#endif

			if(isSafe)
			{
				auto newWay = std::make_shared<ExecuteHeroChain>(path, hero);

				newWay->evaluationContext.strategicalValue = armyValue;
				waysToVisitObj.push_back(newWay);
			}
		}

		if(waysToVisitObj.empty())
			continue;

		for(auto way : waysToVisitObj)
		{
			if(ai->nullkiller->arePathHeroesLocked(way->getPath()))
				continue;

			if(ai->nullkiller->getHeroLockedReason(way->hero.get()) == HeroLockedReason::STARTUP)
				continue;

			way->evaluationContext.closestWayRatio = 1;

			tasks.push_back(sptr(*way));
		}
	}

	return tasks;
}
