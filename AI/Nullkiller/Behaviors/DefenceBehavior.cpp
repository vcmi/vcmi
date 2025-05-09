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

namespace NKAI
{

const float THREAT_IGNORE_RATIO = 2;

using namespace Goals;

std::string DefenceBehavior::toString() const
{
	return "Defend towns";
}

Goals::TGoalVec DefenceBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;
		
	for(auto town : ai->cb->getTownsInfo())
	{
		evaluateDefence(tasks, town, ai);
	}

	return tasks;
}

bool isThreatUnderControl(const CGTownInstance * town, const HitMapInfo & threat, const Nullkiller * ai, const std::vector<AIPath> & paths)
{
	int dayOfWeek = ai->cb->getDate(Date::DAY_OF_WEEK);

	for(const AIPath & path : paths)
	{
		bool threatIsWeak = path.getHeroStrength() / (float)threat.danger > THREAT_IGNORE_RATIO;
		bool needToSaveGrowth = threat.turn == 0 && dayOfWeek == 7;

		if(threatIsWeak && !needToSaveGrowth)
		{
			if((path.exchangeCount == 1 && path.turn() < threat.turn)
				|| path.turn() < threat.turn - 1
				|| (path.turn() < threat.turn && threat.turn >= 2))
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace(
					"Hero %s can eliminate danger for town %s using path %s.",
					path.targetHero->getObjectName(),
					town->getObjectName(),
					path.toString());
#endif

				return true;
			}
		}
	}

	return false;
}

void handleCounterAttack(
	const CGTownInstance * town,
	const HitMapInfo & threat,
	const HitMapInfo & maximumDanger,
	const Nullkiller * ai,
	Goals::TGoalVec & tasks)
{
	if(threat.hero.validAndSet()
		&& threat.turn <= 1
		&& (threat.danger == maximumDanger.danger || threat.turn < maximumDanger.turn))
	{
		auto heroCapturingPaths = ai->pathfinder->getPathInfo(threat.hero->visitablePos());
		auto goals = CaptureObjectsBehavior::getVisitGoals(heroCapturingPaths, ai, threat.hero.get());

		for(int i = 0; i < heroCapturingPaths.size(); i++)
		{
			AIPath & path = heroCapturingPaths[i];
			TSubgoal goal = goals[i];

			if(!goal || goal->invalid() || !goal->isElementar()) continue;

			Composition composition;

			composition.addNext(DefendTown(town, threat, path, true)).addNext(goal);

			tasks.push_back(Goals::sptr(composition));
		}
	}
}

bool handleGarrisonHeroFromPreviousTurn(const CGTownInstance * town, Goals::TGoalVec & tasks, const Nullkiller * ai)
{
	if(ai->isHeroLocked(town->getGarrisonHero()))
	{
		logAi->trace(
			"Hero %s in garrison of town %s is supposed to defend the town",
			town->getGarrisonHero()->getNameTranslated(),
			town->getNameTranslated());

		return true;
	}

	if(!town->getVisitingHero())
	{
		if(ai->cb->getHeroCount(ai->playerID, false) < GameConstants::MAX_HEROES_PER_PLAYER)
		{
			logAi->trace(
				"Extracting hero %s from garrison of town %s",
				town->getGarrisonHero()->getNameTranslated(),
				town->getNameTranslated());

			tasks.push_back(Goals::sptr(Goals::ExchangeSwapTownHeroes(town, nullptr).setpriority(5)));

			return false;
		}
		else if(ai->heroManager->getHeroRole(town->getGarrisonHero()) == HeroRole::MAIN)
		{
			auto armyDismissLimit = 1000;
			auto heroToDismiss = ai->heroManager->findWeakHeroToDismiss(armyDismissLimit);

			if(heroToDismiss)
			{
				tasks.push_back(Goals::sptr(Goals::DismissHero(heroToDismiss).setpriority(5)));

				return false;
			}
		}
	}

	return false;
}

void DefenceBehavior::evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town, const Nullkiller * ai) const
{
#if NKAI_TRACE_LEVEL >= 1
	logAi->trace("Evaluating defence for %s", town->getNameTranslated());
#endif

	auto threatNode = ai->dangerHitMap->getObjectThreat(town);
	std::vector<HitMapInfo> threats = ai->dangerHitMap->getTownThreats(town);
	
	threats.push_back(threatNode.fastestDanger); // no guarantee that fastest danger will be there

	if (town->getGarrisonHero() && handleGarrisonHeroFromPreviousTurn(town, tasks, ai))
	{
		return;
	}
	if(!threatNode.fastestDanger.hero)
	{
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("No threat found for town %s", town->getNameTranslated());
#endif
		return;
	}
	
	uint64_t reinforcement = ai->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town);

	if(reinforcement)
	{
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("Town %s can buy defence army %lld", town->getNameTranslated(), reinforcement);
#endif
		tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(0.5f)));
	}

	auto paths = ai->pathfinder->getPathInfo(town->visitablePos());

	for(auto & threat : threats)
	{
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace(
			"Town %s has threat %lld in %s turns, hero: %s",
			town->getNameTranslated(),
			threat.danger,
			std::to_string(threat.turn),
			threat.hero ? threat.hero->getNameTranslated() : std::string("<no hero>"));
#endif
		handleCounterAttack(town, threat, threatNode.maximumDanger, ai, tasks);

		if(isThreatUnderControl(town, threat, ai, paths))
		{
			continue;
		}

		evaluateRecruitingHero(tasks, threat, town, ai);

		if(paths.empty())
		{
#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("No ways to defend town %s", town->getNameTranslated());
#endif

			continue;
		}

		std::vector<int> pathsToDefend;
		std::map<const CGHeroInstance *, std::vector<int>> defferedPaths;
		AIPath* closestWay = nullptr;

		for(int i = 0; i < paths.size(); i++)
		{
			auto & path = paths[i];

			if (!closestWay || path.movementCost() < closestWay->movementCost())
				closestWay = &path;

#if NKAI_TRACE_LEVEL >= 1
			logAi->trace(
				"Hero %s can defend town with force %lld in %s turns, cost: %f, path: %s",
				path.targetHero->getObjectName(),
				path.getHeroStrength(),
				std::to_string(path.turn()),
				path.movementCost(),
				path.toString());
#endif

			auto townDefenseStrength = town->getGarrisonHero()
				? town->getGarrisonHero()->getTotalStrength()
				: (town->getVisitingHero() ? town->getVisitingHero()->getTotalStrength() : town->getUpperArmy()->getArmyStrength());

			if(town->getVisitingHero() && path.targetHero == town->getVisitingHero())
			{
				if(path.getHeroStrength() < townDefenseStrength)
					continue;
			}
			else if(town->getGarrisonHero() && path.targetHero == town->getGarrisonHero())
			{
				if(path.getHeroStrength() < townDefenseStrength)
					continue;
			}

			if(path.turn() <= threat.turn - 2)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Defer defence of %s by %s because he has enough time to reach the town next turn",
					town->getObjectName(),
					path.targetHero->getObjectName());
#endif

				defferedPaths[path.targetHero].push_back(i);

				continue;
			}

			if (!path.targetHero->canBeMergedWith(*town))
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Can't merge armies of hero %s and town %s",
					path.targetHero->getObjectName(),
					town->getObjectName());
#endif
				continue;
			}

			if(path.targetHero == town->getVisitingHero() && path.exchangeCount == 1)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Put %s to garrison of town %s",
					path.targetHero->getObjectName(),
					town->getObjectName());
#endif

				// dismiss creatures we are not able to pick to be able to hide in garrison
				if(town->getGarrisonHero()
					|| town->getUpperArmy()->stacksCount() == 0
					|| path.targetHero->canBeMergedWith(*town)
					|| (town->getUpperArmy()->getArmyStrength() < 500 && town->fortLevel() >= CGTownInstance::CITADEL))
				{
					tasks.push_back(
						Goals::sptr(Composition()
							.addNext(DefendTown(town, threat, path.targetHero))
							.addNext(ExchangeSwapTownHeroes(town, town->getVisitingHero(), HeroLockedReason::DEFENCE))));
				}

				continue;
			}

			// main without army and visiting scout with army, very specific case
			if(town->getVisitingHero() && town->getUpperArmy()->stacksCount() == 0
				&& path.targetHero != town->getVisitingHero() && path.exchangeCount == 1 && path.turn() == 0
				&& ai->heroManager->evaluateHero(path.targetHero) > ai->heroManager->evaluateHero(town->getVisitingHero())
				&& 10 * path.targetHero->getTotalStrength() < town->getVisitingHero()->getTotalStrength())
			{
				path.heroArmy = town->getVisitingHero();

				tasks.push_back(
					Goals::sptr(Composition()
						.addNext(DefendTown(town, threat, path))
						.addNextSequence({
								sptr(ExchangeSwapTownHeroes(town, town->getVisitingHero())),
								sptr(ExecuteHeroChain(path, town)),
								sptr(ExchangeSwapTownHeroes(town, path.targetHero, HeroLockedReason::DEFENCE))
							})));

				continue;
			}
				
			if(threat.turn == 0 || (path.turn() <= threat.turn && path.getHeroStrength() * ai->settings->getSafeAttackRatio() >= threat.danger))
			{
				if(ai->arePathHeroesLocked(path))
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
			Composition composition;

			composition.addNext(DefendTown(town, threat, path));
			TGoalVec sequence;

			if(town->getGarrisonHero() && path.targetHero == town->getGarrisonHero() && path.exchangeCount == 1)
			{
				composition.addNext(ExchangeSwapTownHeroes(town, town->getGarrisonHero(), HeroLockedReason::DEFENCE));
				tasks.push_back(Goals::sptr(composition));

#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Locking hero %s in garrison of %s",
					town->getGarrisonHero()->getObjectName(),
					town->getObjectName());
#endif

				continue;
			}
			else if(town->getVisitingHero() && path.targetHero != town->getVisitingHero() && !path.containsHero(town->getVisitingHero()))
			{
				if(town->getGarrisonHero() && town->getGarrisonHero() != path.targetHero)
				{
#if NKAI_TRACE_LEVEL >= 1
					logAi->trace("Cancel moving %s to defend town %s as the town has garrison hero",
						path.targetHero->getObjectName(),
						town->getObjectName());
#endif
					continue;
				}
				else if(path.turn() == 0)
				{
					sequence.push_back(sptr(ExchangeSwapTownHeroes(town, town->getVisitingHero())));
				}
			}

#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Move %s to defend town %s",
					path.targetHero->getObjectName(),
					town->getObjectName());
#endif

			ExecuteHeroChain heroChain(path, town);
				
			if (closestWay)
			{
				heroChain.closestWayRatio = closestWay->movementCost() / heroChain.getPath().movementCost();
			}

			sequence.push_back(sptr(heroChain));
			composition.addNextSequence(sequence);

			auto firstBlockedAction = path.getFirstBlockedAction();
			if(firstBlockedAction)
			{
				auto subGoal = firstBlockedAction->decompose(ai, path.targetHero);

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

void DefenceBehavior::evaluateRecruitingHero(Goals::TGoalVec & tasks, const HitMapInfo & threat, const CGTownInstance * town, const Nullkiller * ai) const
{
	if (threat.turn > 0 || town->getGarrisonHero() || town->getVisitingHero())
		return;
	
	if(town->hasBuilt(BuildingID::TAVERN)
		&& ai->cb->getResourceAmount(EGameResID::GOLD) > GameConstants::HERO_GOLD_COST)
	{
		auto heroesInTavern = ai->cb->getAvailableHeroes(town);

		for(auto hero : heroesInTavern)
		{
			if(hero->getTotalStrength() < threat.danger)
				continue;

			bool heroAlreadyHiredInOtherTown = false;
			for (const auto& task : tasks) 
			{
				if (auto recruitGoal = dynamic_cast<Goals::RecruitHero*>(task.get())) 
				{
					if (recruitGoal->getHero() == hero)
					{
						heroAlreadyHiredInOtherTown = true;
						break;
					}
				}
			}
			if (heroAlreadyHiredInOtherTown)
				continue;

			auto myHeroes = ai->cb->getHeroesInfo();

#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Hero %s can be recruited to defend %s", hero->getObjectName(), town->getObjectName());
#endif
			bool needSwap = false;
			const CGHeroInstance * heroToDismiss = nullptr;

			if(town->getVisitingHero())
			{
				if(!town->getGarrisonHero())
					needSwap = true;
				else
				{
					if(town->getVisitingHero()->getArmyStrength() < town->getGarrisonHero()->getArmyStrength())
					{
						if(town->getVisitingHero()->getArmyStrength() >= hero->getArmyStrength())
							continue;

						heroToDismiss = town->getVisitingHero();
					}
					else if(town->getGarrisonHero()->getArmyStrength() >= hero->getArmyStrength())
						continue;
					else
					{
						needSwap = true;
						heroToDismiss = town->getGarrisonHero();
					}
				}

				// avoid dismissing one weak hero in order to recruit another.
				if(heroToDismiss && heroToDismiss->getArmyStrength() + 500 > hero->getArmyStrength())
					continue;
			}
			else if(ai->heroManager->heroCapReached())
			{
				heroToDismiss = ai->heroManager->findWeakHeroToDismiss(hero->getArmyStrength(), town);

				if(!heroToDismiss)
					continue;
			}

			TGoalVec sequence;
			Goals::Composition recruitHeroComposition;

			if(needSwap)
				sequence.push_back(sptr(ExchangeSwapTownHeroes(town, town->getVisitingHero())));

			if(heroToDismiss)
				sequence.push_back(sptr(DismissHero(heroToDismiss)));

			sequence.push_back(sptr(Goals::RecruitHero(town, hero)));

			tasks.push_back(sptr(Goals::Composition().addNext(DefendTown(town, threat, hero)).addNextSequence(sequence)));
		}
	}
}

}
