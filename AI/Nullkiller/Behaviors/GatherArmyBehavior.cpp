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
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/Composition.h"
#include "../Goals/RecruitHero.h"
#include "../Markers/HeroExchange.h"
#include "../Markers/ArmyUpgrade.h"
#include "GatherArmyBehavior.h"
#include "../AIUtility.h"
#include "../Goals/ExchangeSwapTownHeroes.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

std::string GatherArmyBehavior::toString() const
{
	return "Gather army";
}

Goals::TGoalVec GatherArmyBehavior::decompose() const
{
	Goals::TGoalVec tasks;

	auto heroes = cb->getHeroesInfo();

	if(heroes.empty())
	{
		return tasks;
	}

	for(const CGHeroInstance * hero : heroes)
	{
		if(ai->nullkiller->heroManager->getHeroRole(hero) == HeroRole::MAIN)
		{
			vstd::concatenate(tasks, deliverArmyToHero(hero));
		}
	}

	auto towns = cb->getTownsInfo();

	for(const CGTownInstance * town : towns)
	{
		vstd::concatenate(tasks, upgradeArmy(town));
	}

	return tasks;
}

Goals::TGoalVec GatherArmyBehavior::deliverArmyToHero(const CGHeroInstance * hero) const
{
	Goals::TGoalVec tasks;
	const int3 pos = hero->visitablePos();
	auto targetHeroScore = ai->nullkiller->heroManager->evaluateHero(hero);

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Checking ways to gaher army for hero %s, %s", hero->getObjectName(), pos.toString());
#endif

	auto paths = ai->nullkiller->pathfinder->getPathInfo(pos);

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Gather army found %d paths", paths.size());
#endif

	for(const AIPath & path : paths)
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Path found %s, %s, %lld", path.toString(), path.targetHero->getObjectName(), path.heroArmy->getArmyStrength());
#endif
		
		if(path.containsHero(hero))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Selfcontaining path. Ignore");
#endif
			continue;
		}

		bool garrisoned = false;

		if(path.turn() == 0 && hero->inTownGarrison)
		{
#if NKAI_TRACE_LEVEL >= 1
			garrisoned = true;
#endif
		}

		if(path.turn() > 0 && ai->nullkiller->dangerHitMap->enemyCanKillOurHeroesAlongThePath(path))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Target hero can be killed by enemy. Our power %lld", path.heroArmy->getArmyStrength());
#endif
			continue;
		}

		if(ai->nullkiller->arePathHeroesLocked(path))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path because of locked hero");
#endif
			continue;
		}

		HeroExchange heroExchange(hero, path);

		float armyValue = (float)heroExchange.getReinforcementArmyStrength() / hero->getArmyStrength();

		// avoid transferring very small amount of army
		if(armyValue < 0.1f && armyValue < 20000)
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Army value is too small.");
#endif
			continue;
		}

		// avoid trying to move bigger army to the weaker one.
		bool hasOtherMainInPath = false;

		for(auto node : path.nodes)
		{
			if(!node.targetHero) continue;

			auto heroRole = ai->nullkiller->heroManager->getHeroRole(node.targetHero);

			if(heroRole == HeroRole::MAIN)
			{
				auto score = ai->nullkiller->heroManager->evaluateHero(node.targetHero);

				if(score >= targetHeroScore)
				{
					hasOtherMainInPath = true;

					break;
				}
			}
		}

		if(hasOtherMainInPath)
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Army value is too large.");
#endif
			continue;
		}

		auto danger = path.getTotalDanger();
		auto isSafe = isSafeToVisit(hero, path.heroArmy, danger);

#if NKAI_TRACE_LEVEL >= 2
		logAi->trace(
			"It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld",
			isSafe ? "safe" : "not safe",
			hero->getObjectName(),
			path.targetHero->getObjectName(),
			path.getHeroStrength(),
			danger,
			path.getTotalArmyLoss());
#endif

		if(isSafe)
		{
			Composition composition;
			ExecuteHeroChain exchangePath(path, hero);

			exchangePath.closestWayRatio = 1;

			composition.addNext(heroExchange);

			if(garrisoned && path.turn() == 0)
			{
				auto lockReason = ai->nullkiller->getHeroLockedReason(hero);

				composition.addNextSequence({
					sptr(ExchangeSwapTownHeroes(hero->visitedTown)),
					sptr(exchangePath),
					sptr(ExchangeSwapTownHeroes(hero->visitedTown, hero, lockReason))
				});
			}
			else
			{
				composition.addNext(exchangePath);
			}

			auto blockedAction = path.getFirstBlockedAction();

			if(blockedAction)
			{
	#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("Action is blocked. Considering decomposition.");
	#endif
				auto subGoal = blockedAction->decompose(path.targetHero);

				if(subGoal->invalid())
				{
#if NKAI_TRACE_LEVEL >= 1
					logAi->trace("Path is invalid. Skipping");
#endif
					continue;
				}

				composition.addNext(subGoal);
			}
			
			tasks.push_back(sptr(composition));
		}
	}

	return tasks;
}

Goals::TGoalVec GatherArmyBehavior::upgradeArmy(const CGTownInstance * upgrader) const
{
	Goals::TGoalVec tasks;
	const int3 pos = upgrader->visitablePos();
	TResources availableResources = ai->nullkiller->getFreeResources();

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Checking ways to upgrade army in town %s, %s", upgrader->getObjectName(), pos.toString());
#endif
	
	auto paths = ai->nullkiller->pathfinder->getPathInfo(pos);
	std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Found %d paths", paths.size());
#endif

	bool hasMainAround = false;

	for(const AIPath & path : paths)
	{
		auto heroRole = ai->nullkiller->heroManager->getHeroRole(path.targetHero);

		if(heroRole == HeroRole::MAIN && path.turn() < SCOUT_TURN_DISTANCE_LIMIT)
			hasMainAround = true;
	}

	for(const AIPath & path : paths)
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Path found %s, %s, %lld", path.toString(), path.targetHero->getObjectName(), path.heroArmy->getArmyStrength());
#endif
		if(upgrader->visitingHero && (upgrader->visitingHero.get() != path.targetHero || path.exchangeCount == 1))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Town has visiting hero.");
#endif
			continue;
		}

		if(ai->nullkiller->arePathHeroesLocked(path))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path because of locked hero");
#endif
			continue;
		}

		if(path.getFirstBlockedAction())
		{
#if NKAI_TRACE_LEVEL >= 2
			// TODO: decomposition?
			logAi->trace("Ignore path. Action is blocked.");
#endif
			continue;
		}

		auto heroRole = ai->nullkiller->heroManager->getHeroRole(path.targetHero);

		if(heroRole == HeroRole::SCOUT
			&& ai->nullkiller->dangerHitMap->enemyCanKillOurHeroesAlongThePath(path))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Target hero can be killed by enemy. Our power %lld", path.heroArmy->getArmyStrength());
#endif
			continue;
		}

		auto upgrade = ai->nullkiller->armyManager->calculateCreaturesUpgrade(path.heroArmy, upgrader, availableResources);

		if(!upgrader->garrisonHero
			&& (
				hasMainAround
				|| ai->nullkiller->heroManager->getHeroRole(path.targetHero) == HeroRole::MAIN))
		{
			ArmyUpgradeInfo armyToGetOrBuy;

			armyToGetOrBuy.addArmyToGet(
				ai->nullkiller->armyManager->getBestArmy(
					path.targetHero,
					path.heroArmy,
					upgrader->getUpperArmy()));

			armyToGetOrBuy.addArmyToBuy(
				ai->nullkiller->armyManager->toSlotInfo(
					ai->nullkiller->armyManager->getArmyAvailableToBuy(
						path.heroArmy,
						upgrader,
						ai->nullkiller->getFreeResources(),
						path.turn())));

			upgrade.upgradeValue += armyToGetOrBuy.upgradeValue;
			upgrade.upgradeCost += armyToGetOrBuy.upgradeCost;
			vstd::concatenate(upgrade.resultingArmy, armyToGetOrBuy.resultingArmy);

			auto getOrBuyArmyValue = (float)armyToGetOrBuy.upgradeValue / path.getHeroStrength();

			if(!upgrade.upgradeValue
				&& armyToGetOrBuy.upgradeValue > 20000
				&& ai->nullkiller->heroManager->canRecruitHero(town)
				&& path.turn() < SCOUT_TURN_DISTANCE_LIMIT)
			{
				for(auto hero : cb->getAvailableHeroes(town))
				{
					auto scoutReinforcement =  ai->nullkiller->armyManager->howManyReinforcementsCanBuy(hero, town)
						+ ai->nullkiller->armyManager->howManyReinforcementsCanGet(hero, town);

					if(scoutReinforcement >= armyToGetOrBuy.upgradeValue
						&& ai->nullkiller->getFreeGold() >20000
						&& ai->nullkiller->buildAnalyzer->getGoldPreasure() < MAX_GOLD_PEASURE)
					{
						Composition recruitHero;

						recruitHero.addNext(ArmyUpgrade(path.targetHero, town, armyToGetOrBuy)).addNext(RecruitHero(town, hero));
					}
				}
			}
		}

		auto armyValue = (float)upgrade.upgradeValue / path.getHeroStrength();

		if((armyValue < 0.1f && upgrade.upgradeValue < 20000) || upgrade.upgradeValue < 300) // avoid small upgrades
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Army value is too small (%f)", armyValue);
#endif
			continue;
		}

		auto danger = path.getTotalDanger();

		auto isSafe = isSafeToVisit(path.targetHero, path.heroArmy, danger);

#if NKAI_TRACE_LEVEL >= 2
		logAi->trace(
			"It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld",
			isSafe ? "safe" : "not safe",
			upgrader->getObjectName(),
			path.targetHero->getObjectName(),
			path.getHeroStrength(),
			danger,
			path.getTotalArmyLoss());
#endif

		if(isSafe)
		{
			ExecuteHeroChain newWay(path, upgrader);
			
			newWay.closestWayRatio = 1;

			tasks.push_back(sptr(Composition().addNext(ArmyUpgrade(path, upgrader, upgrade)).addNext(newWay)));
		}
	}

	return tasks;
}

}
