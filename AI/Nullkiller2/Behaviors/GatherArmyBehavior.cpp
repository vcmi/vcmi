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
#include "../AIUtility.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/Composition.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/RecruitHero.h"
#include "../Markers/ArmyUpgrade.h"
#include "../Markers/HeroExchange.h"
#include "CaptureObjectsBehavior.h"
#include "GatherArmyBehavior.h"

namespace NK2AI
{

using namespace Goals;

std::string GatherArmyBehavior::toString() const
{
	return "Gather army";
}

Goals::TGoalVec GatherArmyBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	auto heroes = aiNk->cc->getHeroesInfo();
	if(heroes.empty())
	{
		return tasks;
	}

	for(const CGHeroInstance * hero : heroes)
	{
		if(aiNk->heroManager->getHeroRoleOrDefaultInefficient(hero) == HeroRole::MAIN)
		{
			vstd::concatenate(tasks, deliverArmyToHero(aiNk, hero));
		}
	}

	auto towns = aiNk->cc->getTownsInfo();
	for(const CGTownInstance * town : towns)
	{
		vstd::concatenate(tasks, upgradeArmy(aiNk, town));
	}

	return tasks;
}

Goals::TGoalVec GatherArmyBehavior::deliverArmyToHero(const Nullkiller * aiNk, const CGHeroInstance * receiverHero) const
{
	Goals::TGoalVec tasks;
	const int3 pos = receiverHero->visitablePos();
	auto targetHeroScore = aiNk->heroManager->evaluateHero(receiverHero);

#if NK2AI_TRACE_LEVEL >= 1
	logAi->trace("GatherArmyBehavior::deliverArmyToHero Checking ways to gaher army for hero %s, %s", receiverHero->getObjectName(), pos.toString());
#endif

	auto paths = aiNk->pathfinder->getPathInfo(pos, aiNk->isObjectGraphAllowed());

#if NK2AI_TRACE_LEVEL >= 1
	logAi->trace("GatherArmyBehavior::deliverArmyToHero Gather army found %d paths", paths.size());
#endif

	for(const AIPath & path : paths)
	{
#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace(
			"GatherArmyBehavior::deliverArmyToHero Path found %s, %s, %lld", path.toString(), path.targetHero->getObjectName(), path.heroArmy->getArmyStrength()
		);
#endif

		if(path.targetHero->getOwner() != aiNk->playerID)
			continue;

		if(path.containsHero(receiverHero))
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Selfcontaining path. Ignore");
#endif
			continue;
		}

		if(aiNk->arePathHeroesLocked(path))
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Ignore path because of locked hero");
#endif
			continue;
		}

		HeroExchange heroExchange(receiverHero, path);
		// TODO: Mircea: Artifacts (inventory things) aren't considered in this calculation, though they are properly changed in an army exchange, to revisit
		const uint64_t additionalArmyStrength = heroExchange.getReinforcementArmyStrength(aiNk);
		const float additionalArmyRatio = static_cast<float>(additionalArmyStrength) / receiverHero->getArmyStrength();

		// avoid transferring very small amount of army
		if((additionalArmyRatio < 0.1f && additionalArmyStrength < 20000) || additionalArmyStrength < 500)
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Army value is too small.");
#endif
			continue;
		}

		// avoid trying to move bigger army to the weaker one.
		bool hasOtherMainInPath = false;
		for(const auto & node : path.nodes)
		{
			if(!node.targetHero)
				continue;

			if(aiNk->heroManager->getHeroRoleOrDefaultInefficient(node.targetHero) == MAIN)
			{
				const auto score = aiNk->heroManager->evaluateHero(node.targetHero);
				if(score >= targetHeroScore)
				{
					hasOtherMainInPath = true;
					break;
				}
			}
		}

		if(hasOtherMainInPath)
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("GatherArmyBehavior::deliverArmyToHero Avoid trying to move bigger army to the weaker one");
#endif
			continue;
		}

		auto danger = path.getTotalDanger();
		auto isSafe = isSafeToVisit(receiverHero, path.heroArmy, danger, aiNk->settings->getSafeAttackRatio());

#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace(
			"GatherArmyBehavior::deliverArmyToHero It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld",
			isSafe ? "safe" : "not safe",
			receiverHero->getObjectName(),
			path.targetHero->getObjectName(),
			path.getHeroStrength(),
			danger,
			path.getTotalArmyLoss()
		);
#endif

		if(isSafe)
		{
			Composition composition;
			ExecuteHeroChain exchangePath(path, receiverHero);
			exchangePath.closestWayRatio = 1;
			composition.addNext(heroExchange);

			if(receiverHero->isGarrisoned() && path.turn() == 0)
			{
				auto lockReason = aiNk->getHeroLockedReason(receiverHero);

				if(path.targetHero->getVisitedTown() == receiverHero->getVisitedTown())
				{
					composition.addNextSequence({sptr(ExchangeSwapTownHeroes(receiverHero->getVisitedTown(), receiverHero, lockReason))});
				}
				else
				{
					composition.addNextSequence(
						{sptr(ExchangeSwapTownHeroes(receiverHero->getVisitedTown())),
						 sptr(exchangePath),
						 sptr(ExchangeSwapTownHeroes(receiverHero->getVisitedTown(), receiverHero, lockReason))}
					);
				}
			}
			else
			{
				composition.addNext(exchangePath);
			}

			const auto blockedAction = path.getFirstBlockedAction();
			if(blockedAction)
			{
#if NK2AI_TRACE_LEVEL >= 2
				logAi->trace("GatherArmyBehavior::deliverArmyToHero Action is blocked. Considering decomposition.");
#endif
				auto subGoal = blockedAction->decompose(aiNk, path.targetHero);

				if(subGoal->invalid())
				{
#if NK2AI_TRACE_LEVEL >= 1
					logAi->trace("GatherArmyBehavior::deliverArmyToHero Path is invalid. Skipping");
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

Goals::TGoalVec GatherArmyBehavior::upgradeArmy(const Nullkiller * aiNk, const CGTownInstance * upgrader) const
{
	Goals::TGoalVec tasks;
	const int3 pos = upgrader->visitablePos();
	TResources availableResources = aiNk->getFreeResources();

#if NK2AI_TRACE_LEVEL >= 1
	logAi->trace("Checking ways to upgrade army in town %s, %s", upgrader->getObjectName(), pos.toString());
#endif

	auto paths = aiNk->pathfinder->getPathInfo(pos, aiNk->isObjectGraphAllowed());
	auto goals = CaptureObjectsBehavior::getVisitGoals(paths, aiNk);

	std::vector<std::shared_ptr<ExecuteHeroChain>> waysToVisitObj;

#if NK2AI_TRACE_LEVEL >= 1
	logAi->trace("Found %d paths", paths.size());
#endif

	bool hasMainAround = false;
	for(const AIPath & path : paths)
	{
		auto heroRole = aiNk->heroManager->getHeroRoleOrDefaultInefficient(path.targetHero);
		if(heroRole == HeroRole::MAIN && path.turn() < aiNk->settings->getScoutHeroTurnDistanceLimit())
		{
			hasMainAround = true;
			break;
		}
	}

	for(int i = 0; i < paths.size(); i++)
	{
		auto & path = paths[i];
		auto visitGoal = goals[i];

#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace("Path found %s, %s, %lld", path.toString(), path.targetHero->getObjectName(), path.heroArmy->getArmyStrength());
#endif

		if(visitGoal->invalid())
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Not valid way.");
#endif
			continue;
		}

		if(upgrader->getVisitingHero() && (upgrader->getVisitingHero() != path.targetHero || path.exchangeCount == 1))
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Town has visiting hero.");
#endif
			continue;
		}

		if(aiNk->arePathHeroesLocked(path))
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path because of locked hero");
#endif
			continue;
		}

		if(path.getFirstBlockedAction())
		{
#if NK2AI_TRACE_LEVEL >= 2
			// TODO: decomposition?
			logAi->trace("Ignore path. Action is blocked.");
#endif
			continue;
		}

		auto upgrade = aiNk->armyManager->calculateCreaturesUpgrade(path.heroArmy, upgrader, availableResources);
		if(!upgrader->getGarrisonHero() && (hasMainAround || aiNk->heroManager->getHeroRoleOrDefaultInefficient(path.targetHero) == HeroRole::MAIN))
		{
			ArmyUpgradeInfo armyToGetOrBuy;
			armyToGetOrBuy.addArmyToGet(aiNk->armyManager->getBestArmy(path.targetHero, path.heroArmy, upgrader->getUpperArmy(), TerrainId::NONE));
			armyToGetOrBuy.upgradeValue -= path.heroArmy->getArmyStrength();

			upgrade.upgradeValue += armyToGetOrBuy.upgradeValue;
			upgrade.upgradeCost += armyToGetOrBuy.upgradeCost;
			vstd::concatenate(upgrade.resultingArmy, armyToGetOrBuy.resultingArmy);

			if(!upgrade.upgradeValue && armyToGetOrBuy.upgradeValue > 20000 // TODO: Mircea: Move to constant
			   && aiNk->heroManager->canRecruitHero(upgrader)
			   && path.turn() < aiNk->settings->getScoutHeroTurnDistanceLimit())
			{
				for(const auto * const hero : aiNk->cc->getAvailableHeroes(upgrader))
				{
					auto scoutReinforcement = aiNk->armyManager->howManyReinforcementsCanGet(hero, upgrader);
					if(scoutReinforcement >= armyToGetOrBuy.upgradeValue && aiNk->getFreeGold() > 20000 // TODO: Mircea: Move to constant
					   && !aiNk->buildAnalyzer->isGoldPressureOverMax())
					{
						Composition recruitHero;
						recruitHero.addNext(ArmyUpgrade(path.targetHero, town, armyToGetOrBuy)).addNext(RecruitHero(upgrader, hero));
					}
				}
			}
		}

		auto armyValue = static_cast<float>(upgrade.upgradeValue) / path.getHeroStrength();
		// TODO: Mircea: Move to constants
		if((armyValue < 0.25f && upgrade.upgradeValue < 40000) || upgrade.upgradeValue < 2000) // avoid small upgrades
		{
#if NK2AI_TRACE_LEVEL >= 2
			logAi->trace("Ignore path. Army value is too small (%f)", armyValue);
#endif
			continue;
		}

		auto danger = path.getTotalDanger();
		auto isSafe = isSafeToVisit(path.targetHero, path.heroArmy, danger, aiNk->settings->getSafeAttackRatio());

#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace(
			"It is %s to visit %s by %s with army %lld, danger %lld and army loss %lld",
			isSafe ? "safe" : "not safe",
			upgrader->getObjectName(),
			path.targetHero->getObjectName(),
			path.getHeroStrength(),
			danger,
			path.getTotalArmyLoss()
		);
#endif

		if(isSafe)
		{
			tasks.push_back(sptr(Composition().addNext(ArmyUpgrade(path, upgrader, upgrade)).addNext(visitGoal)));
		}
	}

	return tasks;
}

}
