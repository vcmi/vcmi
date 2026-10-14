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
#include "DefenceBehaviorUtils.h"

#include "../../../lib/IGameSettings.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/BuyArmy.h"
#include "../Goals/Composition.h"
#include "../Goals/DismissHero.h"
#include "../Goals/ExchangeSwapTownHeroes.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Goals/RecruitHero.h"
#include "../Markers/DefendTown.h"
#include "CaptureObjectsBehavior.h"
#include "RecruitHeroBehavior.h"

namespace NK2AI
{

const float THREAT_IGNORE_RATIO = 2;

using namespace Goals;

namespace Goals
{
	uint64_t estimateTownFortificationDefence(const CGTownInstance & town, bool hasDefenders)
	{
		if(!hasDefenders)
			return 0;

		if(town.fortLevel() == CGTownInstance::CASTLE)
			return 10000;

		if(town.fortLevel() == CGTownInstance::CITADEL)
			return 4000;

		return 0;
	}

	uint64_t estimateTownDefence(const CGTownInstance & town, const CGHeroInstance * committedDefender)
	{
		uint64_t result = town.getArmyStrength();

		if(committedDefender)
			result = std::max(result, committedDefender->getTotalStrength());

		return result + estimateTownFortificationDefence(town, committedDefender && result > 0);
	}

	bool isTownDefenceSufficient(uint64_t defenceStrength, const HitMapInfo & threat, float safeAttackRatio)
	{
		if(threat.danger == 0)
			return true;

		const auto requiredDefence = threat.turn == 0 ? static_cast<float>(threat.danger) : threat.danger * safeAttackRatio;

		return defenceStrength >= requiredDefence;
	}

	bool shouldLockTownDefender(const CGTownInstance & town, const CGHeroInstance & defender, const HitMapInfo & threat, float safeAttackRatio)
	{
		if(threat.danger == 0 || threat.turn > 1)
			return false;

		if(isTownDefenceSufficient(estimateTownDefence(town, nullptr), threat, safeAttackRatio))
			return false;

		return isTownDefenceSufficient(estimateTownDefence(town, &defender), threat, safeAttackRatio);
	}

	int countTownThreatsCoveredByDefender(const CGTownInstance & town, const CGHeroInstance & defender, const std::vector<HitMapInfo> & threats, float safeAttackRatio)
	{
		int result = 0;
		const auto townDefence = estimateTownDefence(town, nullptr);
		const auto defenceWithHero = estimateTownDefence(town, &defender);

		for(const auto & threat : threats)
		{
			if(threat.danger == 0 || threat.turn > 1)
				continue;

			if(isTownDefenceSufficient(townDefence, threat, safeAttackRatio))
				continue;

			if(isTownDefenceSufficient(defenceWithHero, threat, safeAttackRatio))
				++result;
		}

		return result;
	}

	bool isHeroRequiredForTownDefence(const CGTownInstance & town, const CGHeroInstance & defender, const std::vector<HitMapInfo> & threats, float safeAttackRatio)
	{
		return countTownThreatsCoveredByDefender(town, defender, threats, safeAttackRatio) > 0;
	}

	bool shouldReserveTownDefender(const CGTownInstance & town, const CGHeroInstance & defender, const std::vector<HitMapInfo> & threats, float safeAttackRatio)
	{
		const auto townDefence = estimateTownDefence(town, nullptr);
		const auto defenceWithHero = estimateTownDefence(town, &defender);

		if(defenceWithHero <= townDefence)
			return false;

		for(const auto & threat : threats)
		{
			if(threat.danger == 0 || threat.turn > 1)
				continue;

			if(!isTownDefenceSufficient(townDefence, threat, safeAttackRatio))
				return true;
		}

		return false;
	}

	bool isDefenderReleaseAllowedForTownCapture(
		const CGHeroInstance & defender,
		const CGObjectInstance & target,
		bool targetIsEnemy,
		bool defenderMakesHomeStable,
		uint64_t remainingTownReinforcement,
		int dayOfWeek,
		int daysInWeek)
	{
		if(target.ID != Obj::TOWN || !targetIsEnemy)
			return false;

		if(!defenderMakesHomeStable)
			return dayOfWeek != daysInWeek || remainingTownReinforcement == 0;

		const uint64_t ignoredReinforcement = std::max<uint64_t>(1000, defender.getTotalStrength() / 20);
		if(remainingTownReinforcement > ignoredReinforcement)
			return false;

		return dayOfWeek != daysInWeek || remainingTownReinforcement == 0;
	}

	bool isSafeSameTurnReturnPath(const CGHeroInstance & hero, const AIPath & path, float safeAttackRatio, float availableMovement)
	{
		if(path.targetHero != &hero || path.heroArmy == nullptr)
			return false;

		if(path.turn() != 0 || path.exchangeCount != 1)
			return false;

		if(path.getFirstBlockedAction())
			return false;

		for(const auto & node : path.nodes)
		{
			if(node.targetHero != &hero || node.specialAction)
				return false;
		}

		if(!isSafeToVisit(&hero, path.heroArmy, path.getTotalDanger(), safeAttackRatio))
			return false;

		return path.movementCost() * 2.0f <= availableMovement;
	}

	bool isSafeSameTurnReturnPath(const CGHeroInstance & hero, const AIPath & path, float safeAttackRatio)
	{
		const float movementLimit = std::max(1, hero.movementPointsLimit());
		const float availableMovement = hero.movementPointsRemaining() / movementLimit;

		return isSafeSameTurnReturnPath(hero, path, safeAttackRatio, availableMovement);
	}
}

namespace
{
	constexpr float DEFENSIVE_EMERGENCY_PRIORITY = 1000000.0f;

	uint64_t estimateTownMobileDefence(const CGTownInstance * town)
	{
		uint64_t result = town->getArmyStrength();

		if(const auto * visitingHero = town->getVisitingHero())
			result = std::max(result, visitingHero->getTotalStrength());

		if(const auto * garrisonHero = town->getGarrisonHero())
			result = std::max(result, garrisonHero->getTotalStrength());

		return result;
	}

	bool shouldLockPathDefender(const CGTownInstance * town, const HitMapInfo & threat, const AIPath & path, const Nullkiller * aiNk)
	{
		return path.turn() == 0 && shouldLockTownDefender(*town, *path.targetHero, threat, aiNk->settings->getSafeAttackRatio());
	}

	void setDefensiveEmergencyPriority(Composition & composition, bool emergency)
	{
		if(emergency)
			composition.setpriority(DEFENSIVE_EMERGENCY_PRIORITY);
	}

	uint64_t stableTownDefence(const CGTownInstance * town, const Nullkiller * aiNk)
	{
		uint64_t result = estimateTownDefence(*town, nullptr);

		if(const auto * garrisonHero = town->getGarrisonHero())
		{
			if(aiNk->getHeroLockedReason(garrisonHero) == HeroLockedReason::DEFENCE)
				result = std::max(result, estimateTownDefence(*town, garrisonHero));
		}

		if(const auto * visitingHero = town->getVisitingHero())
		{
			if(aiNk->getHeroLockedReason(visitingHero) == HeroLockedReason::DEFENCE)
				result = std::max(result, estimateTownDefence(*town, visitingHero));
		}

		return result;
	}

	bool hasStableTownDefence(const CGTownInstance * town, const HitMapInfo & threat, const Nullkiller * aiNk)
	{
		if(threat.danger == 0)
			return true;

		return isTownDefenceSufficient(stableTownDefence(town, aiNk), threat, aiNk->settings->getSafeAttackRatio());
	}

	bool containsRecruitHeroTask(const Goals::TSubgoal & task, const CGHeroInstance * hero)
	{
		if(const auto * recruitGoal = dynamic_cast<const Goals::RecruitHero *>(task.get()))
			return recruitGoal->getHero() == hero;

		if(task->goalType == Goals::COMPOSITION)
		{
			for(const auto & subgoal : task->decompose(nullptr))
			{
				if(containsRecruitHeroTask(subgoal, hero))
					return true;
			}
		}

		return false;
	}
}

std::string DefenceBehavior::toString() const
{
	return "Defend towns";
}

Goals::TGoalVec DefenceBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;

	for(const auto town : aiNk->cc->getTownsInfo())
	{
		evaluateDefence(tasks, town, aiNk);
	}

	return tasks;
}

bool isThreatUnderControl(const CGTownInstance * town, const HitMapInfo & threat, const Nullkiller * aiNk, const std::vector<AIPath> & paths)
{
	int dayOfWeek = aiNk->cc->getCalendar().getDayOfWeek();

	for(const AIPath & path : paths)
	{
		bool threatIsWeak = path.getHeroStrength() / (float)threat.danger > THREAT_IGNORE_RATIO;
		bool needToSaveGrowth = threat.turn == 0 && dayOfWeek == LIBRARY->engineSettings()->getInteger(EGameSettings::GENERAL_DAYS_PER_WEEK);

		if(threatIsWeak && !needToSaveGrowth)
		{
			if((path.exchangeCount == 1 && path.turn() < threat.turn) || path.turn() < threat.turn - 1 || (path.turn() < threat.turn && threat.turn >= 2))
			{
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace(
					"Hero %s can eliminate danger for town %s using path %s.", path.targetHero->getNameTextID(), town->getNameTextID(), path.toString()
				);
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
	const Nullkiller * aiNk,
	Goals::TGoalVec & tasks
)
{
	if(!hasStableTownDefence(town, threat, aiNk))
		return;

	if(threat.heroPtr.isVerified() && threat.turn <= 1 && (threat.danger == maximumDanger.danger || threat.turn < maximumDanger.turn))
	{
		auto heroCapturingPaths = aiNk->pathfinder->getPathInfo(threat.heroPtr->visitablePos());
		auto goals = CaptureObjectsBehavior::getVisitGoals(heroCapturingPaths, aiNk, threat.heroPtr.get());

		for(int i = 0; i < heroCapturingPaths.size(); i++)
		{
			AIPath & path = heroCapturingPaths[i];
			TSubgoal goal = goals[i];

			if(!goal || goal->invalid() || !goal->isElementar())
				continue;

			Composition composition;
			composition.addNext(DefendTown(town, threat, path, true)).addNext(goal);
			tasks.push_back(Goals::sptr(composition));
		}
	}
}

void handleGarrisonReturnCounterAttack(
	const CGTownInstance * town,
	const HitMapInfo & threat,
	const HitMapInfo & maximumDanger,
	const Nullkiller * aiNk,
	Goals::TGoalVec & tasks)
{
	const auto * garrisonHero = town->getGarrisonHero();

	if(!garrisonHero)
		return;

	const auto lockReason = aiNk->getHeroLockedReason(garrisonHero);
	if(lockReason != HeroLockedReason::NOT_LOCKED && lockReason != HeroLockedReason::DEFENCE)
		return;

	if(threat.heroPtr.isVerified() && threat.turn <= 1 && (threat.danger == maximumDanger.danger || threat.turn < maximumDanger.turn))
	{
		auto heroCapturingPaths = aiNk->pathfinder->getPathInfo(threat.heroPtr->visitablePos());

		for(const auto & path : heroCapturingPaths)
		{
			if(!isSafeSameTurnReturnPath(*garrisonHero, path, aiNk->settings->getSafeAttackRatio()))
				continue;

			Composition composition;
			TGoalVec sequence;
			sequence.push_back(sptr(ExchangeSwapTownHeroes(town, nullptr)));
			sequence.push_back(sptr(ExecuteHeroChain(path, threat.heroPtr.get())));
			sequence.push_back(sptr(ExchangeSwapTownHeroes(town, garrisonHero, HeroLockedReason::DEFENCE)));

			composition.addNext(DefendTown(town, threat, path, true)).addNextSequence(sequence);
			setDefensiveEmergencyPriority(composition, shouldLockTownDefender(*town, *garrisonHero, threat, aiNk->settings->getSafeAttackRatio()));
			tasks.push_back(Goals::sptr(composition));
		}
	}
}

bool handleGarrisonHeroFromPreviousTurn(const CGTownInstance * town, Goals::TGoalVec & tasks, const Nullkiller * aiNk, const std::vector<HitMapInfo> & threats)
{
	const auto * garrisonHero = town->getGarrisonHero();

	if(aiNk->isHeroLocked(garrisonHero) || shouldReserveTownDefender(*town, *garrisonHero, threats, aiNk->settings->getSafeAttackRatio()))
	{
		logAi->trace("Hero %s in garrison of town %s is supposed to defend the town", garrisonHero->getNameTextID(), town->getNameTextID());
		return true;
	}

	if(!town->getVisitingHero())
	{
		if(aiNk->cc->getHeroCount(aiNk->playerID, false) < GameConstants::MAX_HEROES_PER_PLAYER)
		{
			logAi->trace("Extracting hero %s from garrison of town %s", garrisonHero->getNameTextID(), town->getNameTextID());
			tasks.push_back(Goals::sptr(Goals::ExchangeSwapTownHeroes(town, nullptr).setpriority(5)));
			return false;
		}

		if(aiNk->heroManager->getHeroRoleOrDefaultInefficient(garrisonHero) == HeroRole::MAIN)
		{
			auto armyDismissLimit = 1000;
			auto heroToDismiss = aiNk->heroManager->findWeakHeroToDismiss(armyDismissLimit);
			if(heroToDismiss)
			{
				tasks.push_back(Goals::sptr(Goals::DismissHero(heroToDismiss).setpriority(5)));
				return false;
			}
		}
	}

	return false;
}

void DefenceBehavior::evaluateDefence(Goals::TGoalVec & tasks, const CGTownInstance * town, const Nullkiller * aiNk) const
{
#if NK2AI_TRACE_LEVEL >= 1
	logAi->trace("Evaluating defence for %s", town->getNameTranslated());
#endif

	auto threatNode = aiNk->dangerHitMap->getObjectThreat(town);
	std::vector<HitMapInfo> threats = aiNk->dangerHitMap->getTownThreats(town);
	// TODO: Mircea: Why don't we check if there's any danger in threadNode? Maybe map is still unexplored and no danger
	// or simply no one is around
	threats.push_back(threatNode.fastestDanger); // no guarantee that fastest danger will be there

	for(const auto & threat : threats)
		handleGarrisonReturnCounterAttack(town, threat, threatNode.maximumDanger, aiNk, tasks);

	if(town->getGarrisonHero() && handleGarrisonHeroFromPreviousTurn(town, tasks, aiNk, threats))
		return;

	if(!threatNode.fastestDanger.heroPtr.isVerified())
	{
#if NK2AI_TRACE_LEVEL >= 1
		logAi->trace("No threat found for town %s", town->getNameTranslated());
#endif
		return;
	}

	const uint64_t reinforcement = aiNk->armyManager->howManyReinforcementsCanBuy(town->getUpperArmy(), town);
	if(reinforcement)
	{
#if NK2AI_TRACE_LEVEL >= 1
		logAi->trace("Town %s can buy defence army %lld", town->getNameTranslated(), reinforcement);
#endif

		// TODO: Mircea: This won't have any money left because BuyArmyBehavior runs first and could have used all resources by now
		tasks.push_back(Goals::sptr(Goals::BuyArmy(town, reinforcement).setpriority(0.5f)));
	}

	auto paths = aiNk->pathfinder->getPathInfo(town->visitablePos());

	for(auto & threat : threats)
	{
#if NK2AI_TRACE_LEVEL >= 1
		logAi->trace(
			"Town %s has threat %lld in %s turns, hero: %s",
			town->getNameTranslated(),
			threat.danger,
			std::to_string(threat.turn),
			threat.heroPtr.nameOrDefault()
		);
#endif
		handleCounterAttack(town, threat, threatNode.maximumDanger, aiNk, tasks);

		if(isThreatUnderControl(town, threat, aiNk, paths))
			continue;

		evaluateRecruitingHero(tasks, threat, town, aiNk);

		if(paths.empty())
		{
#if NK2AI_TRACE_LEVEL >= 1
			logAi->trace("No ways to defend town %s", town->getNameTranslated());
#endif
			continue;
		}

		std::vector<int> pathsToDefend;
		HeroMap<std::vector<int>> defferedPaths;
		AIPath * closestWay = nullptr;

		for(int i = 0; i < paths.size(); i++)
		{
			auto & path = paths[i];
			if(!closestWay || path.movementCost() < closestWay->movementCost())
				closestWay = &path;

#if NK2AI_TRACE_LEVEL >= 1
			logAi->trace(
				"Hero %s can defend town with force %lld in %s turns, cost: %f, path: %s",
				path.targetHero->getNameTextID(),
				path.getHeroStrength(),
				std::to_string(path.turn()),
				path.movementCost(),
				path.toString()
			);
#endif

			const auto townDefenseStrength = estimateTownMobileDefence(town);
			const bool lockDefenderNow = shouldLockPathDefender(town, threat, path, aiNk);

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
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace(
					"Defer defence of %s by %s because he has enough time to reach the town next turn", town->getNameTextID(), path.targetHero->getNameTextID()
				);
#endif

				defferedPaths[path.targetHero].push_back(i);
				continue;
			}

			if(!path.targetHero->canBeMergedWith(*town))
			{
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Can't merge armies of hero %s and town %s", path.targetHero->getNameTextID(), town->getNameTextID());
#endif
				continue;
			}

			if(path.targetHero == town->getVisitingHero() && path.exchangeCount == 1)
			{
#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Put %s to garrison of town %s", path.targetHero->getNameTextID(), town->getNameTextID());
#endif

				// dismiss creatures we are not able to pick to be able to hide in garrison
				if(town->getGarrisonHero() || town->getUpperArmy()->stacksCount() == 0 || path.targetHero->canBeMergedWith(*town)
				   || (town->getUpperArmy()->getArmyStrength() < 500 && town->fortLevel() >= CGTownInstance::CITADEL))
				{
					Composition composition;
					composition.addNext(DefendTown(town, threat, path.targetHero))
						.addNext(ExchangeSwapTownHeroes(town, town->getVisitingHero(), HeroLockedReason::DEFENCE));

					setDefensiveEmergencyPriority(composition, lockDefenderNow);
					tasks.push_back(Goals::sptr(composition));
				}

				continue;
			}

			// main without army and visiting scout with army, very specific case
			if(town->getVisitingHero() && town->getUpperArmy()->stacksCount() == 0 && path.targetHero != town->getVisitingHero() && path.exchangeCount == 1
			   && path.turn() == 0 && aiNk->heroManager->evaluateHero(path.targetHero) > aiNk->heroManager->evaluateHero(town->getVisitingHero())
			   && 10 * path.targetHero->getTotalStrength() < town->getVisitingHero()->getTotalStrength())
			{
				path.heroArmy = town->getVisitingHero();

				tasks.push_back(
					Goals::sptr(
						Composition()
							.addNext(DefendTown(town, threat, path))
							.addNextSequence(
								{sptr(ExchangeSwapTownHeroes(town, town->getVisitingHero())),
								 sptr(ExecuteHeroChain(path, town)),
								 sptr(ExchangeSwapTownHeroes(town, path.targetHero, HeroLockedReason::DEFENCE))}
							)
					)
				);

				continue;
			}

			const bool heroStrengthCoversThreat = path.turn() <= threat.turn && path.getHeroStrength() >= threat.danger * aiNk->settings->getSafeAttackRatio();
			if(threat.turn == 0 || lockDefenderNow || heroStrengthCoversThreat)
			{
				if(aiNk->arePathHeroesLocked(path))
				{
#if NK2AI_TRACE_LEVEL >= 1
					logAi->trace("Can not move %s to defend town %s. Path is locked.", path.targetHero->getNameTextID(), town->getNameTextID());

#endif
					continue;
				}

				pathsToDefend.push_back(i);
			}
		}

		for(int i : pathsToDefend)
		{
			AIPath & path = paths[i];
			const bool lockDefenderNow = shouldLockPathDefender(town, threat, path, aiNk);

			for(int j : defferedPaths[path.targetHero])
			{
				AIPath & defferedPath = paths[j];
				if(defferedPath.getHeroStrength() >= path.getHeroStrength() && defferedPath.turn() <= path.turn())
				{
					continue; // TODO: Mircea: Should it be break instead? Or continue for the outside loop?
				}
			}

			Composition composition;
			composition.addNext(DefendTown(town, threat, path));
			TGoalVec sequence;

			if(town->getGarrisonHero() && path.targetHero == town->getGarrisonHero() && path.exchangeCount == 1)
			{
				composition.addNext(ExchangeSwapTownHeroes(town, town->getGarrisonHero(), HeroLockedReason::DEFENCE));
				setDefensiveEmergencyPriority(composition, lockDefenderNow);
				tasks.push_back(Goals::sptr(composition));

#if NK2AI_TRACE_LEVEL >= 1
				logAi->trace("Locking hero %s in garrison of %s", town->getGarrisonHero()->getObjectNameTextID(), town->getNameTextID());
#endif
				continue;
			}

			if(town->getVisitingHero() && path.targetHero != town->getVisitingHero() && !path.containsHero(town->getVisitingHero()))
			{
				if(town->getGarrisonHero() && town->getGarrisonHero() != path.targetHero)
				{
#if NK2AI_TRACE_LEVEL >= 1
					logAi->trace("Cancel moving %s to defend town %s as the town has garrison hero", path.targetHero->getNameTextID(), town->getNameTextID());
#endif
					continue;
				}

				if(path.turn() == 0)
				{
					sequence.push_back(sptr(ExchangeSwapTownHeroes(town, town->getVisitingHero())));
				}
			}

#if NK2AI_TRACE_LEVEL >= 1
			logAi->trace("Move %s to defend town %s", path.targetHero->getNameTextID(), town->getNameTextID());
#endif

			ExecuteHeroChain heroChain(path, town);

			if(closestWay)
			{
				heroChain.closestWayRatio = closestWay->movementCost() / heroChain.getPath().movementCost();
			}

			sequence.push_back(sptr(heroChain));

			if(lockDefenderNow)
				sequence.push_back(sptr(ExchangeSwapTownHeroes(town, path.targetHero, HeroLockedReason::DEFENCE)));

			composition.addNextSequence(sequence);
			setDefensiveEmergencyPriority(composition, lockDefenderNow);

			const auto firstBlockedAction = path.getFirstBlockedAction();
			if(firstBlockedAction)
			{
				auto subGoal = firstBlockedAction->decompose(aiNk, path.targetHero);

#if NK2AI_TRACE_LEVEL >= 2
				logAi->trace("Decomposing special action %s returns %s", firstBlockedAction->toString(), subGoal->toString());
#endif

				if(subGoal->invalid())
				{
#if NK2AI_TRACE_LEVEL >= 1
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

void DefenceBehavior::evaluateRecruitingHero(Goals::TGoalVec & tasks, const HitMapInfo & threat, const CGTownInstance * town, const Nullkiller * aiNk)
{
	// TODO: Mircea: Replace with aiNk->heroManager->canRecruitHero(town) but skip limit?
	if(town->hasBuilt(BuildingID::TAVERN) && aiNk->cc->getResourceAmount(EGameResID::GOLD) > GameConstants::HERO_GOLD_COST)
	{
		const auto heroesInTavern = aiNk->cc->getAvailableHeroes(town);
		for(auto hero : heroesInTavern)
		{
			if(!RecruitHeroBehavior::isDefensiveRecruitEmergency(*town, *hero, threat, aiNk->settings->getSafeAttackRatio()))
				continue;

			bool heroAlreadyHiredInOtherTown = false;
			for(const auto & task : tasks)
			{
				if(containsRecruitHeroTask(task, hero))
				{
					heroAlreadyHiredInOtherTown = true;
					break;
				}
			}
			if(heroAlreadyHiredInOtherTown)
				continue;

			auto myHeroes = aiNk->cc->getHeroesInfo();

#if NK2AI_TRACE_LEVEL >= 1
			logAi->trace("Hero %s can be recruited to defend %s", hero->getNameTextID(), town->getNameTextID());
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
				// TODO: Mircea: Move to constant
				if(heroToDismiss && heroToDismiss->getArmyStrength() + 500 > hero->getArmyStrength())
					continue;
			}
			// TODO: Mircea: Check if it immediately dismisses after losing a castle, though that implies losing a hero too if present in the castle
			else if(aiNk->heroManager->heroCapReached())
			{
				heroToDismiss = aiNk->heroManager->findWeakHeroToDismiss(hero->getArmyStrength(), town);
				if(!heroToDismiss)
					continue;
			}

			TGoalVec sequence;

			if(needSwap)
				sequence.push_back(sptr(ExchangeSwapTownHeroes(town, town->getVisitingHero())));

			if(heroToDismiss)
				sequence.push_back(sptr(DismissHero(heroToDismiss)));

			sequence.push_back(sptr(Goals::RecruitHero(town, hero)));
			sequence.push_back(sptr(ExchangeSwapTownHeroes(town, hero, HeroLockedReason::DEFENCE)));

			Goals::Composition composition;
			composition.addNext(DefendTown(town, threat, hero)).addNextSequence(sequence);
			composition.setpriority(DEFENSIVE_EMERGENCY_PRIORITY);
			tasks.push_back(sptr(composition));
		}
	}
}

}
