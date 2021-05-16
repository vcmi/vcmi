/*
* Nullkiller.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Nullkiller.h"
#include "../VCAI.h"
#include "../AIhelper.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Behaviors/RecruitHeroBehavior.h"
#include "../Behaviors/BuyArmyBehavior.h"
#include "../Behaviors/StartupBehavior.h"
#include "../Behaviors/DefenceBehavior.h"
#include "../Behaviors/BuildingBehavior.h"
#include "../Behaviors/GatherArmyBehavior.h"
#include "../Goals/Invalid.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;

Nullkiller::Nullkiller()
{
	priorityEvaluator.reset(new PriorityEvaluator());
	dangerHitMap.reset(new DangerHitMapAnalyzer());
	buildAnalyzer.reset(new BuildAnalyzer());
}

Goals::TTask Nullkiller::choseBestTask(Goals::TTaskVec & tasks) const
{
	Goals::TTask bestTask = *vstd::maxElementByFun(tasks, [](Goals::TTask task) -> float{
		return task->priority;
	});

	return bestTask;
}

Goals::TTask Nullkiller::choseBestTask(Goals::TSubgoal behavior) const
{
	logAi->debug("Checking behavior %s", behavior->toString());

	const int MAX_DEPTH = 0;
	Goals::TGoalVec goals[MAX_DEPTH + 1];
	Goals::TTaskVec tasks;
	std::map<Goals::TSubgoal, Goals::TSubgoal> decompositionMap;

	goals[0] = {behavior};

	int depth = 0;
	while(goals[0].size())
	{
		TSubgoal current = goals[depth].back();

#if AI_TRACE_LEVEL >= 1
		logAi->trace("Decomposing %s, level: %d", current->toString(), depth);
#endif

		TGoalVec subgoals = current->decompose();

#if AI_TRACE_LEVEL >= 1
		logAi->trace("Found %d goals", subgoals.size());
#endif

		if(depth < MAX_DEPTH)
		{
			goals[depth + 1].clear();
		}

		for(auto subgoal : subgoals)
		{
			if(subgoal->isElementar())
			{
				auto task = taskptr(*subgoal);

#if AI_TRACE_LEVEL >= 1
		logAi->trace("Found task %s", task->toString());
#endif

				if(task->priority <= 0)
					task->priority = priorityEvaluator->evaluate(subgoal);

				tasks.push_back(task);
			}
			else if(depth < MAX_DEPTH)
			{
#if AI_TRACE_LEVEL >= 1
				logAi->trace("Found abstract goal %s", subgoal->toString());
#endif
				goals[depth + 1].push_back(subgoal);
			}
		}

		if(depth < MAX_DEPTH && goals[depth + 1].size())
		{
			depth++;
		}
		else
		{
			goals[depth].pop_back();

			while(depth > 0 && goals[depth].empty())
			{
				depth--;
				goals[depth].pop_back();
			}
		}
	}

	if(tasks.empty())
	{
		logAi->debug("Behavior %s found no tasks", behavior->toString());

		return Goals::taskptr(Goals::Invalid());
	}

	auto task = choseBestTask(tasks);

	logAi->debug("Behavior %s returns %s, priority %f", behavior->toString(), task->toString(), task->priority);

	return task;
}

void Nullkiller::resetAiState()
{
	lockedHeroes.clear();
	dangerHitMap->reset();
}

void Nullkiller::updateAiState()
{
	activeHero = nullptr;

	ai->validateVisitableObjs();
	dangerHitMap->updateHitMap();

	// TODO: move to hero manager
	auto activeHeroes = ai->getMyHeroes();

	vstd::erase_if(activeHeroes, [this](const HeroPtr & hero) -> bool
	{
		auto lockedReason = getHeroLockedReason(hero.h);

		return lockedReason == HeroLockedReason::DEFENCE;
	});

	ai->ah->updatePaths(activeHeroes, true);
	ai->ah->update();

	buildAnalyzer->update();
}

bool Nullkiller::isHeroLocked(const CGHeroInstance * hero) const
{
	return getHeroLockedReason(hero) != HeroLockedReason::NOT_LOCKED;
}

bool Nullkiller::arePathHeroesLocked(const AIPath & path) const
{
	if(getHeroLockedReason(path.targetHero) == HeroLockedReason::STARTUP)
	{
#if AI_TRACE_LEVEL >= 1
		logAi->trace("Hero %s is locked by STARTUP. Discarding %s", path.targetHero->name, path.toString());
#endif
		return true;
	}

	for(auto & node : path.nodes)
	{
		auto lockReason = getHeroLockedReason(node.targetHero);

		if(lockReason != HeroLockedReason::NOT_LOCKED)
		{
#if AI_TRACE_LEVEL >= 1
			logAi->trace("Hero %s is locked by STARTUP. Discarding %s", path.targetHero->name, path.toString());
#endif
			return true;
		}
	}

	return false;
}

HeroLockedReason Nullkiller::getHeroLockedReason(const CGHeroInstance * hero) const
{
	auto found = lockedHeroes.find(hero);

	return found != lockedHeroes.end() ? found->second : HeroLockedReason::NOT_LOCKED;
}

void Nullkiller::makeTurn()
{
	resetAiState();

	while(true)
	{
		updateAiState();

		Goals::TTaskVec bestTasks = {
			choseBestTask(sptr(BuyArmyBehavior())),
			choseBestTask(sptr(CaptureObjectsBehavior())),
			choseBestTask(sptr(RecruitHeroBehavior())),
			choseBestTask(sptr(DefenceBehavior())),
			choseBestTask(sptr(BuildingBehavior())),
			choseBestTask(sptr(GatherArmyBehavior()))
		};

		if(cb->getDate(Date::DAY) == 1)
		{
			bestTasks.push_back(choseBestTask(sptr(StartupBehavior())));
		}

		Goals::TTask bestTask = choseBestTask(bestTasks);

		/*if(bestTask->invalid())
		{
			logAi->trace("No goals found. Ending turn.");

			return;
		}*/

		if(bestTask->priority < MIN_PRIORITY)
		{
			logAi->trace("Goal %s has too low priority. It is not worth doing it. Ending turn.", bestTask->toString());

			return;
		}

		logAi->debug("Trying to realize %s (value %2.3f)", bestTask->toString(), bestTask->priority);

		try
		{
			bestTask->accept(ai.get());
		}
		catch(goalFulfilledException &)
		{
			logAi->trace("Task %s completed", bestTask->toString());
		}
		catch(std::exception & e)
		{
			logAi->debug("Failed to realize subgoal of type %s, I will stop.", bestTask->toString());
			logAi->debug("The error message was: %s", e.what());

			return;
		}
	}
}
