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
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"
#include "../Goals/BuyArmy.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Goals/RecruitHero.h"
#include "../Goals/DismissHero.h"
#include "../Goals/Composition.h"
#include "../Goals/CaptureObject.h"
#include "../Markers/DefendTown.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

const double TREAT_IGNORE_RATIO = 0.5;

using namespace Goals;

std::string DefenceBehavior::toString() const
{
	return "Defend towns";
}

Goals::TGoalVec DefenceBehavior::decompose() const
{
	Goals::TGoalVec tasks;
		
	for(auto town : cb->getTownsInfo())
	{
		evaluateDefence(tasks, town);
	}

	return tasks;
}

void DefenceBehavior::evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town) const
{
	logAi->trace("Evaluating defence for %s", town->getNameTranslated());

	auto treatNode = ai->nullkiller->dangerHitMap->getObjectTreat(town);
	auto treats = { treatNode.maximumDanger, treatNode.fastestDanger };

	if(!treatNode.fastestDanger.hero)
	{
		logAi->trace("No treat found for town %s", town->getNameTranslated());

		return;
	}

	int dayOfWeek = cb->getDate(Date::DAY_OF_WEEK);

	if(town->garrisonHero)
	{
		if(!ai->nullkiller->isHeroLocked(town->garrisonHero.get()))
		{
			if(!town->visitingHero && cb->getHeroCount(ai->playerID, false) < GameConstants::MAX_HEROES_PER_PLAYER)
			{
				logAi->trace(
					"Extracting hero %s from garrison of town %s",
					town->garrisonHero->getNameTranslated(),
					town->getNameTranslated());

				tasks.push_back(Goals::sptr(Goals::ExchangeSwapTownHeroes(town, nullptr).setpriority(5)));

				return;
			}
		}

		logAi->trace(
			"Hero %s in garrison of town %s is suposed to defend the town",
			town->garrisonHero->getNameTranslated(),
			town->getNameTranslated());

		return;
	}
	
	uint64_t reinforcement = ai->nullkiller->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town);

	if(reinforcement)
	{
		logAi->trace("Town %s can buy defence army %lld", town->getNameTranslated(), reinforcement);
		tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(0.5f)));
	}

	auto paths = ai->nullkiller->pathfinder->getPathInfo(town->visitablePos());

	for(auto & treat : treats)
	{
		logAi->trace(
			"Town %s has treat %lld in %s turns, hero: %s",
			town->getNameTranslated(),
			treat.danger,
			std::to_string(treat.turn),
			treat.hero->getNameTranslated());

		bool treatIsUnderControl = false;

		for(AIPath & path : paths)
		{
			if(town->visitingHero && path.targetHero != town->visitingHero.get())
				continue;

			if(town->visitingHero && path.getHeroStrength() < town->visitingHero->getHeroStrength())
				continue;

			if(treat.hero.validAndSet()
				&& treat.turn <= 1
				&& (treat.danger == treatNode.maximumDanger.danger || treat.turn < treatNode.maximumDanger.turn)
				&& isSafeToVisit(path.targetHero, path.heroArmy, treat.danger))
			{
				Composition composition;

				composition.addNext(DefendTown(town, treat, path)).addNext(CaptureObject(treat.hero.get()));

				tasks.push_back(Goals::sptr(composition));
			}

			bool treatIsWeak = path.getHeroStrength() / treat.danger > TREAT_IGNORE_RATIO;
			bool needToSaveGrowth = treat.turn == 0 && dayOfWeek == 7;

			if(treatIsWeak && !needToSaveGrowth)
			{
				if((path.exchangeCount == 1 && path.turn() < treat.turn)
					|| path.turn() < treat.turn - 1
					|| (path.turn() < treat.turn && treat.turn >= 2))
				{
#if NKAI_TRACE_LEVEL >= 1
					logAi->trace(
						"Hero %s can eliminate danger for town %s using path %s.",
						path.targetHero->getObjectName(),
						town->getObjectName(),
						path.toString());
#endif

					treatIsUnderControl = true;

					break;
				}
			}
		}

		if(treatIsUnderControl)
			continue;

		if(!town->visitingHero
			&& town->hasBuilt(BuildingID::TAVERN)
			&& cb->getResourceAmount(Res::GOLD) > GameConstants::HERO_GOLD_COST)
		{
			auto heroesInTavern = cb->getAvailableHeroes(town);

			for(auto hero : heroesInTavern)
			{
				if(hero->getTotalStrength() > treat.danger)
				{
					auto myHeroes = cb->getHeroesInfo();

					if(cb->getHeroesInfo().size() < ALLOWED_ROAMING_HEROES)
					{
#if NKAI_TRACE_LEVEL >= 1
						logAi->trace("Hero %s can be recruited to defend %s", hero->getObjectName(), town->getObjectName());
#endif
						tasks.push_back(Goals::sptr(Goals::RecruitHero(town, hero).setpriority(1)));
						continue;
					}
					else
					{
						const CGHeroInstance * weakestHero = nullptr;

						for(auto existingHero : myHeroes)
						{
							if(ai->nullkiller->isHeroLocked(existingHero)
								|| existingHero->getArmyStrength() > hero->getArmyStrength()
								|| ai->nullkiller->heroManager->getHeroRole(existingHero) == HeroRole::MAIN
								|| existingHero->movement
								|| existingHero->artifactsWorn.size() > (existingHero->hasSpellbook() ? 2 : 1))
								continue;

							if(!weakestHero || weakestHero->getFightingStrength() > existingHero->getFightingStrength())
							{
								weakestHero = existingHero;
							}

							if(weakestHero)
							{
								tasks.push_back(Goals::sptr(Goals::DismissHero(weakestHero)));
							}
						}
					}
				}
			}
		}

		if(paths.empty())
		{
			logAi->trace("No ways to defend town %s", town->getNameTranslated());

			continue;
		}

		std::vector<int> pathsToDefend;
		std::map<const CGHeroInstance *, std::vector<int>> defferedPaths;

		for(int i = 0; i < paths.size(); i++)
		{
			auto & path = paths[i];

#if NKAI_TRACE_LEVEL >= 1
			logAi->trace(
				"Hero %s can defend town with force %lld in %s turns, cost: %f, path: %s",
				path.targetHero->getObjectName(),
				path.getHeroStrength(),
				std::to_string(path.turn()),
				path.movementCost(),
				path.toString());
#endif
			if(path.turn() <= treat.turn - 2)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Defer defence of %s by %s because he has enough time to reach the town next trun",
					town->getObjectName(),
					path.targetHero->getObjectName());
#endif

				defferedPaths[path.targetHero].push_back(i);

				continue;
			}

			if(path.targetHero == town->visitingHero && path.exchangeCount == 1)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Put %s to garrison of town %s",
					path.targetHero->getObjectName(),
					town->getObjectName());
#endif

				// dismiss creatures we are not able to pick to be able to hide in garrison
				if(town->garrisonHero
					|| town->getUpperArmy()->stacksCount() == 0
					|| (town->getUpperArmy()->getArmyStrength() < 500 && town->fortLevel() >= CGTownInstance::CITADEL))
				{
					tasks.push_back(
						Goals::sptr(Composition()
							.addNext(DefendTown(town, treat, path.targetHero))
							.addNext(ExchangeSwapTownHeroes(town, town->visitingHero.get(), HeroLockedReason::DEFENCE))));
				}

				continue;
			}
				
			if(treat.turn == 0 || (path.turn() <= treat.turn && path.getHeroStrength() * SAFE_ATTACK_CONSTANT >= treat.danger))
			{
				if(ai->nullkiller->arePathHeroesLocked(path))
				{
#if NKAI_TRACE_LEVEL >= 1
					logAi->trace("Can not move %s to defend town %s. Path is locked.",
						path.targetHero->getObjectName(),
						town->getObjectName());

#endif
					continue;
				}

				pathsToDefend.push_back(i);
			}
		}

		for(int i : pathsToDefend)
		{
			AIPath & path = paths[i];

			for(int j : defferedPaths[path.targetHero])
			{
				AIPath & defferedPath = paths[j];

				if(defferedPath.getHeroStrength() >= path.getHeroStrength()
					&& defferedPath.turn() <= path.turn())
				{
					continue;
				}
			}

#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Move %s to defend town %s",
				path.targetHero->getObjectName(),
				town->getObjectName());
#endif
			Composition composition;

			composition.addNext(DefendTown(town, treat, path)).addNext(ExecuteHeroChain(path, town));

			auto firstBlockedAction = path.getFirstBlockedAction();
			if(firstBlockedAction)
			{
				auto subGoal = firstBlockedAction->decompose(path.targetHero);

#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Decomposing special action %s returns %s", firstBlockedAction->toString(), subGoal->toString());
#endif

				if(subGoal->invalid())
				{
#if NKAI_TRACE_LEVEL >= 1
					logAi->trace("Path is invalid. Skipping");
#endif
					continue;
				}

				composition.addNext(subGoal);
			}

			tasks.push_back(Goals::sptr(composition));
		}
	}

	logAi->debug("Found %d tasks", tasks.size());
}

}
