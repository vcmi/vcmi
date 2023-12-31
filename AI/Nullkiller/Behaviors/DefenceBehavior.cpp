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

Goals::TGoalVec DefenceBehavior::decompose() const
{
	Goals::TGoalVec tasks;
		
	for(auto town : cb->getTownsInfo())
	{
		evaluateDefence(tasks, town);
	}

	return tasks;
}

bool isThreatUnderControl(const CGTownInstance * town, const HitMapInfo & threat, const std::vector<AIPath> & paths)
{
	int dayOfWeek = cb->getDate(Date::DAY_OF_WEEK);

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
	Goals::TGoalVec & tasks)
{
	if(threat.hero.validAndSet()
		&& threat.turn <= 1
		&& (threat.danger == maximumDanger.danger || threat.turn < maximumDanger.turn))
	{
		auto heroCapturingPaths = ai->nullkiller->pathfinder->getPathInfo(threat.hero->visitablePos());
		auto goals = CaptureObjectsBehavior::getVisitGoals(heroCapturingPaths, threat.hero.get());

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

bool handleGarrisonHeroFromPreviousTurn(const CGTownInstance * town, Goals::TGoalVec & tasks)
{
	if(ai->nullkiller->isHeroLocked(town->garrisonHero.get()))
	{
		logAi->trace(
			"Hero %s in garrison of town %s is supposed to defend the town",
			town->garrisonHero->getNameTranslated(),
			town->getNameTranslated());

		return true;
	}

	if(!town->visitingHero)
	{
		if(cb->getHeroCount(ai->playerID, false) < GameConstants::MAX_HEROES_PER_PLAYER)
		{
			logAi->trace(
				"Extracting hero %s from garrison of town %s",
				town->garrisonHero->getNameTranslated(),
				town->getNameTranslated());

			tasks.push_back(Goals::sptr(Goals::ExchangeSwapTownHeroes(town, nullptr).setpriority(5)));

			return true;
		}
		else if(ai->nullkiller->heroManager->getHeroRole(town->garrisonHero.get()) == HeroRole::MAIN)
		{
			auto armyDismissLimit = 1000;
			auto heroToDismiss = ai->nullkiller->heroManager->findWeakHeroToDismiss(armyDismissLimit);

			if(heroToDismiss)
			{
				tasks.push_back(Goals::sptr(Goals::DismissHero(heroToDismiss).setpriority(5)));

				return true;
			}
		}
	}

	return false;
}

void DefenceBehavior::evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town) const
{
	logAi->trace("Evaluating defence for %s", town->getNameTranslated());

	auto threatNode = ai->nullkiller->dangerHitMap->getObjectThreat(town);
	std::vector<HitMapInfo> threats = ai->nullkiller->dangerHitMap->getTownThreats(town);
	
	threats.push_back(threatNode.fastestDanger); // no guarantee that fastest danger will be there

	if(town->garrisonHero && handleGarrisonHeroFromPreviousTurn(town, tasks))
	{
		return;
	}

	if(!threatNode.fastestDanger.hero)
	{
		logAi->trace("No threat found for town %s", town->getNameTranslated());

		return;
	}
	
	uint64_t reinforcement = ai->nullkiller->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town);

	if(reinforcement)
	{
		logAi->trace("Town %s can buy defence army %lld", town->getNameTranslated(), reinforcement);
		tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(0.5f)));
	}

	auto paths = ai->nullkiller->pathfinder->getPathInfo(town->visitablePos());

	for(auto & threat : threats)
	{
		logAi->trace(
			"Town %s has threat %lld in %s turns, hero: %s",
			town->getNameTranslated(),
			threat.danger,
			std::to_string(threat.turn),
			threat.hero ? threat.hero->getNameTranslated() : std::string("<no hero>"));

		handleCounterAttack(town, threat, threatNode.maximumDanger, tasks);

		if(isThreatUnderControl(town, threat, paths))
		{
			continue;
		}

		evaluateRecruitingHero(tasks, threat, town);

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

			auto townDefenseStrength = town->garrisonHero
				? town->garrisonHero->getTotalStrength()
				: (town->visitingHero ? town->visitingHero->getTotalStrength() : town->getUpperArmy()->getArmyStrength());

			if(town->visitingHero && path.targetHero == town->visitingHero.get())
			{
				if(path.getHeroStrength() < townDefenseStrength)
					continue;
			}
			else if(town->garrisonHero && path.targetHero == town->garrisonHero.get())
			{
				if(path.getHeroStrength() < townDefenseStrength)
					continue;
			}

			if(path.turn() <= threat.turn - 2)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Defer defence of %s by %s because he has enough time to reach the town next trun",
					town->getObjectName(),
					path.targetHero->getObjectName());
#endif

				defferedPaths[path.targetHero].push_back(i);

				continue;
			}

			if(path.targetHero == town->visitingHero.get() && path.exchangeCount == 1)
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
							.addNext(DefendTown(town, threat, path.targetHero))
							.addNext(ExchangeSwapTownHeroes(town, town->visitingHero.get(), HeroLockedReason::DEFENCE))));
				}

				continue;
			}

			// main without army and visiting scout with army, very specific case
			if(town->visitingHero && town->getUpperArmy()->stacksCount() == 0
				&& path.targetHero != town->visitingHero.get() && path.exchangeCount == 1 && path.turn() == 0
				&& ai->nullkiller->heroManager->evaluateHero(path.targetHero) > ai->nullkiller->heroManager->evaluateHero(town->visitingHero.get())
				&& 10 * path.targetHero->getTotalStrength() < town->visitingHero->getTotalStrength())
			{
				path.heroArmy = town->visitingHero.get();

				tasks.push_back(
					Goals::sptr(Composition()
						.addNext(DefendTown(town, threat, path))
						.addNextSequence({
								sptr(ExchangeSwapTownHeroes(town, town->visitingHero.get())),
								sptr(ExecuteHeroChain(path, town)),
								sptr(ExchangeSwapTownHeroes(town, path.targetHero, HeroLockedReason::DEFENCE))
							})));

				continue;
			}
				
			if(threat.turn == 0 || (path.turn() <= threat.turn && path.getHeroStrength() * SAFE_ATTACK_CONSTANT >= threat.danger))
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
			Composition composition;

			composition.addNext(DefendTown(town, threat, path));
			TGoalVec sequence;

			if(town->garrisonHero && path.targetHero == town->garrisonHero.get() && path.exchangeCount == 1)
			{
				composition.addNext(ExchangeSwapTownHeroes(town, town->garrisonHero.get(), HeroLockedReason::DEFENCE));
				tasks.push_back(Goals::sptr(composition));

#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Locking hero %s in garrison of %s",
					town->garrisonHero.get()->getObjectName(),
					town->getObjectName());
#endif

				continue;
			}
			else if(town->visitingHero && path.targetHero != town->visitingHero && !path.containsHero(town->visitingHero))
			{
				if(town->garrisonHero)
				{
					if(ai->nullkiller->heroManager->getHeroRole(town->visitingHero.get()) == HeroRole::SCOUT
						&& town->visitingHero->getArmyStrength() < path.heroArmy->getArmyStrength() / 20)
					{
						if(path.turn() == 0)
							sequence.push_back(sptr(DismissHero(town->visitingHero.get())));
					}
					else
					{
#if NKAI_TRACE_LEVEL >= 1
						logAi->trace("Cancel moving %s to defend town %s as the town has garrison hero",
							path.targetHero->getObjectName(),
							town->getObjectName());
#endif
						continue;
					}
				}
				else if(path.turn() == 0)
				{
					sequence.push_back(sptr(ExchangeSwapTownHeroes(town, town->visitingHero.get())));
				}
			}

#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Move %s to defend town %s",
					path.targetHero->getObjectName(),
					town->getObjectName());
#endif

			sequence.push_back(sptr(ExecuteHeroChain(path, town)));
			composition.addNextSequence(sequence);

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

void DefenceBehavior::evaluateRecruitingHero(Goals::TGoalVec & tasks, const HitMapInfo & threat, const CGTownInstance * town) const
{
	if(town->hasBuilt(BuildingID::TAVERN)
		&& cb->getResourceAmount(EGameResID::GOLD) > GameConstants::HERO_GOLD_COST)
	{
		auto heroesInTavern = cb->getAvailableHeroes(town);

		for(auto hero : heroesInTavern)
		{
			if(hero->getTotalStrength() < threat.danger)
				continue;

			auto myHeroes = cb->getHeroesInfo();

#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Hero %s can be recruited to defend %s", hero->getObjectName(), town->getObjectName());
#endif
			bool needSwap = false;
			const CGHeroInstance * heroToDismiss = nullptr;

			if(town->visitingHero)
			{
				if(!town->garrisonHero)
					needSwap = true;
				else
				{
					if(town->visitingHero->getArmyStrength() < town->garrisonHero->getArmyStrength())
					{
						if(town->visitingHero->getArmyStrength() >= hero->getArmyStrength())
							continue;

						heroToDismiss = town->visitingHero.get();
					}
					else if(town->garrisonHero->getArmyStrength() >= hero->getArmyStrength())
						continue;
					else
					{
						needSwap = true;
						heroToDismiss = town->garrisonHero.get();
					}
				}
			}
			else if(ai->nullkiller->heroManager->heroCapReached())
			{
				heroToDismiss = ai->nullkiller->heroManager->findWeakHeroToDismiss(hero->getArmyStrength());

				if(!heroToDismiss)
					continue;
			}

			TGoalVec sequence;
			Goals::Composition recruitHeroComposition;

			if(needSwap)
				sequence.push_back(sptr(ExchangeSwapTownHeroes(town, town->visitingHero.get())));

			if(heroToDismiss)
				sequence.push_back(sptr(DismissHero(heroToDismiss)));

			sequence.push_back(sptr(Goals::RecruitHero(town, hero)));

			tasks.push_back(sptr(Goals::Composition().addNext(DefendTown(town, threat, hero)).addNextSequence(sequence)));
		}
	}
}

}
