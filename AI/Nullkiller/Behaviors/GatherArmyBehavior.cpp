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
#include "CaptureObjectsBehavior.h"
#include "../AIUtility.h"
#include "../Goals/ExchangeSwapTownHeroes.h"

namespace NKAI
{

using namespace Goals;

std::string GatherArmyBehavior::toString() const
{
	return "Gather army";
}

Goals::TGoalVec GatherArmyBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;
	const auto heroes = ai->cb->getHeroesInfo();
	if(heroes.empty())
		return tasks;

	for(const CGHeroInstance * hero : heroes)
	{
		if(ai->heroManager->getHeroRole(hero) == HeroRole::MAIN)
		{
			vstd::concatenate(tasks, deliverArmyToHero(ai, hero));
		}
	}

	auto towns = ai->cb->getTownsInfo();
	for(const CGTownInstance * town : towns)
	{
		vstd::concatenate(tasks, upgradeArmy(ai, town));
	}

	return tasks;
}

Goals::TGoalVec GatherArmyBehavior::deliverArmyToHero(const Nullkiller * ai, const CGHeroInstance * hero) const
{
	Goals::TGoalVec tasks;
	const int3 pos = hero->visitablePos();
	auto targetHeroScore = ai->heroManager->evaluateHero(hero);

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("GatherArmyBehavior::deliverArmyToHero Checking ways to gather army for hero %s, %s", hero->getObjectName(), pos.toString());
#endif

	auto paths = ai->pathfinder->getPathInfo(pos, ai->isObjectGraphAllowed());

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("GatherArmyBehavior::deliverArmyToHero Gather army found %d paths", paths.size());
#endif

	for(const AIPath & path : paths)
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("GatherArmyBehavior::deliverArmyToHero Path found %s, %s, %lld", path.toString(), path.targetHero->getObjectName(), path.heroArmy->getArmyStrength());
#endif
		
		if (path.targetHero->getOwner() != ai->playerID)
			continue;
		
		if(path.containsHero(hero))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Ignore because self-containing path");
#endif
			continue;
		}

		if(ai->arePathHeroesLocked(path))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Ignore because of locked hero");
#endif
			continue;
		}

		HeroExchange heroExchange(hero, path);
		uint64_t armyValue = heroExchange.getReinforcementArmyStrength(ai);
		float armyRatio = static_cast<float>(armyValue) / hero->getArmyStrength();

		// avoid transferring very small amount of army
		if((armyRatio < 0.1f && armyValue < 20000) || armyValue < 500)
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Ignore because army value is too small.");
#endif
			continue;
		}

		// avoid trying to move bigger army to the weaker one.
		bool hasOtherMainInPath = false;
		for(auto node : path.nodes)
		{
			if(!node.targetHero) continue;

			if(ai->heroManager->getHeroRole(node.targetHero) == MAIN)
			{
				// TODO: Mircea: Shouldn't >= be replaced with >?
				if(ai->heroManager->evaluateHero(node.targetHero) >= targetHeroScore)
				{
					hasOtherMainInPath = true;
					break;
				}
			}
		}

		if(hasOtherMainInPath)
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Ignore because hasOtherMainInPath");
#endif
			continue;
		}

		auto danger = path.getTotalDanger();
		auto isSafe = isSafeToVisit(hero, path.heroArmy, danger, ai->settings->getSafeAttackRatio());

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

			if(hero->isGarrisoned() && path.turn() == 0)
			{
				auto lockReason = ai->getHeroLockedReason(hero);

				if(path.targetHero->getVisitedTown() == hero->getVisitedTown())
				{
					composition.addNextSequence({
						sptr(ExchangeSwapTownHeroes(hero->getVisitedTown(), hero, lockReason))});
				}
				else
				{
					composition.addNextSequence({
						sptr(ExchangeSwapTownHeroes(hero->getVisitedTown())),
						sptr(exchangePath),
						sptr(ExchangeSwapTownHeroes(hero->getVisitedTown(), hero, lockReason))});
				}
			}
			else
			{
				composition.addNext(exchangePath);
			}

			auto blockedAction = path.getFirstBlockedAction();

			if(blockedAction)
			{
	#if NKAI_TRACE_LEVEL >= 2
				logAi->trace("GatherArmyBehavior::deliverArmyToHero Action is blocked. Considering decomposition.");
	#endif
				auto subGoal = blockedAction->decompose(ai, path.targetHero);

				if(subGoal->invalid())
				{
#if NKAI_TRACE_LEVEL >= 1
					logAi->trace("GatherArmyBehavior::deliverArmyToHero Ignore because path is invalid");
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

Goals::TGoalVec GatherArmyBehavior::upgradeArmy(const Nullkiller * ai, const CGTownInstance * upgrader) const
{
	Goals::TGoalVec tasks;
	const int3 pos = upgrader->visitablePos();
	TResources availableResources = ai->getFreeResources();

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Checking ways to upgrade army in town %s, %s", upgrader->getObjectName(), pos.toString());
#endif
	
	auto paths = ai->pathfinder->getPathInfo(pos, ai->isObjectGraphAllowed());
	auto goals = CaptureObjectsBehavior::getVisitGoals(paths, ai);

	std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;

#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Found %d paths", paths.size());
#endif

	bool hasMainAround = false;

	for(const AIPath & path : paths)
	{
		auto heroRole = ai->heroManager->getHeroRole(path.targetHero);

		if(heroRole == HeroRole::MAIN && path.turn() < ai->settings->getScoutHeroTurnDistanceLimit())
			hasMainAround = true;
	}

	for(int i = 0; i < paths.size(); i++)
	{
		auto & path = paths[i];
		auto visitGoal = goals[i];

#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Path found %s, %s, %lld", path.toString(), path.targetHero->getObjectName(), path.heroArmy->getArmyStrength());
#endif

		if(visitGoal->invalid())
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Not valid way.");
#endif
			continue;
		}

		if(upgrader->getVisitingHero() && (upgrader->getVisitingHero() != path.targetHero || path.exchangeCount == 1))
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Town has visiting hero.");
#endif
			continue;
		}

		if(ai->arePathHeroesLocked(path))
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

		auto upgrade = ai->armyManager->calculateCreaturesUpgrade(path.heroArmy, upgrader, availableResources);

		if(!upgrader->getGarrisonHero()
			&& (
				hasMainAround
				|| ai->heroManager->getHeroRole(path.targetHero) == HeroRole::MAIN))
		{
			ArmyUpgradeInfo armyToGetOrBuy;

			armyToGetOrBuy.addArmyToGet(
				ai->armyManager->getBestArmy(
					path.targetHero,
					path.heroArmy,
					upgrader->getUpperArmy(),
					TerrainId::NONE));

			armyToGetOrBuy.upgradeValue -= path.heroArmy->getArmyStrength();

			upgrade.upgradeValue += armyToGetOrBuy.upgradeValue;
			upgrade.upgradeCost += armyToGetOrBuy.upgradeCost;
			vstd::concatenate(upgrade.resultingArmy, armyToGetOrBuy.resultingArmy);

			if(!upgrade.upgradeValue
				&& armyToGetOrBuy.upgradeValue > 20000
				&& ai->heroManager->canRecruitHero(upgrader)
				&& path.turn() < ai->settings->getScoutHeroTurnDistanceLimit())
			{
				for(auto hero : cb->getAvailableHeroes(upgrader))
				{
					auto scoutReinforcement = ai->armyManager->howManyReinforcementsCanGet(hero, upgrader);

					if(scoutReinforcement >= armyToGetOrBuy.upgradeValue
						&& ai->getFreeGold() >20000
						&& !ai->buildAnalyzer->isGoldPressureHigh())
					{
						Composition recruitHero;

						recruitHero.addNext(ArmyUpgrade(path.targetHero, town, armyToGetOrBuy)).addNext(RecruitHero(upgrader, hero));
					}
				}
			}
		}

		auto armyValue = (float)upgrade.upgradeValue / path.getHeroStrength();

		if((armyValue < 0.25f && upgrade.upgradeValue < 40000) || upgrade.upgradeValue < 2000) // avoid small upgrades
		{
#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Army value is too small (%f)", armyValue);
#endif
			continue;
		}

		auto danger = path.getTotalDanger();

		auto isSafe = isSafeToVisit(path.targetHero, path.heroArmy, danger, ai->settings->getSafeAttackRatio());

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
			tasks.push_back(sptr(Composition().addNext(ArmyUpgrade(path, upgrader, upgrade)).addNext(visitGoal)));
		}
	}

	return tasks;
}

}
