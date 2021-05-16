/*
* DefenceBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "DefenceBehavior.h"
#include "../VCAI.h"
#include "../Engine/Nullkiller.h"
#include "../AIhelper.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Goals/VisitTile.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

std::string DefenceBehavior::toString() const
{
	return "Defend towns";
}

Goals::TGoalVec DefenceBehavior::getTasks()
{
	Goals::TGoalVec tasks;
		
	auto heroes = cb->getHeroesInfo();

	if(heroes.size())
	{
		auto mainArmy = *vstd::maxElementByFun(heroes, [](const CGHeroInstance * hero) -> uint64_t
		{
			return hero->getTotalStrength();
		});

		for(auto town : cb->getTownsInfo())
		{
			evaluateDefence(tasks, town);
		}
	}

	return tasks;
}

void DefenceBehavior::evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town)
{
	logAi->debug("Evaluating defence for %s", town->name);

	auto treatNode = ai->nullkiller->dangerHitMap->getObjectTreat(town);
	auto treats = { treatNode.fastestDanger, treatNode.maximumDanger };

	if(town->garrisonHero)
	{
		if(!ai->nullkiller->isHeroLocked(town->garrisonHero.get()))
		{
			tasks.push_back(Goals::sptr(Goals::ExchangeSwapTownHeroes(town, nullptr).setpriority(5)));
			return;
		}

		logAi->debug(
			"Hero %s in garrison of town %s is suposed to defend the town",
			town->garrisonHero->name,
			town->name);

		return;
	}

	if(town->visitingHero && isSafeToVisit(town->visitingHero.get(), treatNode.maximumDanger.danger))
	{
		logAi->debug(
			"Town %s has visiting hero %s who is strong enough to defend the town", 
			town->name, 
			town->visitingHero->name);

		return;
	}

	uint64_t reinforcement = ai->ah->howManyReinforcementsCanBuy(town->getUpperArmy(), town);

	if(reinforcement)
	{
		logAi->debug("Town %s can buy defence army %lld", town->name, reinforcement);
		tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(0.5f)));
	}

	auto paths = ai->ah->getPathsToTile(town->visitablePos());

	if(paths.empty())
	{
		logAi->debug("No ways to defend town %s", town->name);

		return;
	}

	for(AIPath & path : paths)
	{
		for(auto & treat : treats)
		{
			if(isSafeToVisit(path.targetHero, path.heroArmy, treat.danger))
			{
				logAi->debug(
					"Hero %s can eliminate danger for town %s using path %s.", 
					path.targetHero->name,
					town->name,
					path.toString());

				return;
			}
		}
	}

	for(auto & treat : treats)
	{
		logAi->debug(
			"Town %s has treat %lld in %s turns, hero: %s", 
			town->name,
			treat.danger,
			std::to_string(treat.turn),
			treat.hero->name);

		for(AIPath & path : paths)
		{
#if AI_TRACE_LEVEL >= 1
			logAi->trace(
				"Hero %s can defend town with force %lld in %s turns, path: %s",
				path.targetHero->name,
				path.getHeroStrength(),
				std::to_string(path.turn()),
				path.toString());
#endif

			float priority = 0.6f + (float)path.getHeroStrength() / treat.danger / (treat.turn + 1);

			if(path.targetHero == town->visitingHero && path.exchangeCount == 1)
			{
#if AI_TRACE_LEVEL >= 1
				logAi->trace("Put %s to garrison of town %s with priority %f",
					path.targetHero->name,
					town->name,
					priority);
#endif

				tasks.push_back(Goals::sptr(Goals::ExchangeSwapTownHeroes(town, town->visitingHero.get()).setpriority(priority)));

				continue;
			}
				
			if(path.getHeroStrength() * SAFE_ATTACK_CONSTANT >= treat.danger)
			{
#if AI_TRACE_LEVEL >= 1
				logAi->trace("Move %s to defend town %s with priority %f",
					path.targetHero->name,
					town->name,
					priority);
#endif

				tasks.push_back(Goals::sptr(Goals::ExecuteHeroChain(path, town).setpriority(priority)));

				continue;
			}
		}
	}

	logAi->debug("Found %d tasks", tasks.size());

	/*for(auto & treat : treats)
	{
		auto paths = ai->ah->getPathsToTile(treat.hero->visitablePos());

		for(AIPath & path : paths)
		{
			tasks.push_back(Goals::sptr(Goals::ExecuteHeroChain(path)));
		}
	}*/
}