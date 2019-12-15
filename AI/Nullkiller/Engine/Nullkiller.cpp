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

Nullkiller::Nullkiller()
{
	priorityEvaluator.reset(new PriorityEvaluator());
	dangerHitMap.reset(new DangerHitMapAnalyzer());
	buildAnalyzer.reset(new BuildAnalyzer());
}

Goals::TSubgoal Nullkiller::choseBestTask(Goals::TGoalVec & tasks) const
{
	Goals::TSubgoal bestTask = *vstd::maxElementByFun(tasks, [](Goals::TSubgoal goal) -> float{
		return goal->priority;
	});

	return bestTask;
}

Goals::TSubgoal Nullkiller::choseBestTask(std::shared_ptr<Behavior> behavior) const
{
	logAi->debug("Checking behavior %s", behavior->toString());

	auto tasks = behavior->getTasks();

	if(tasks.empty())
	{
		logAi->debug("Behavior %s found no tasks", behavior->toString());

		return Goals::sptr(Goals::Invalid());
	}

	logAi->trace("Evaluating priorities, tasks count %d", tasks.size());

	for(auto task : tasks)
	{
		task->setpriority(priorityEvaluator->evaluate(task));
	}

	auto task = choseBestTask(tasks);

	logAi->debug("Behavior %s returns %s(%s), priority %f", behavior->toString(), task->name(), task->tile.toString(), task->priority);

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

		Goals::TGoalVec bestTasks = {
			choseBestTask(std::make_shared<BuyArmyBehavior>()),
			choseBestTask(std::make_shared<CaptureObjectsBehavior>()),
			choseBestTask(std::make_shared<RecruitHeroBehavior>()),
			choseBestTask(std::make_shared<DefenceBehavior>()),
			choseBestTask(std::make_shared<BuildingBehavior>()),
			choseBestTask(std::make_shared<GatherArmyBehavior>())
		};

		if(cb->getDate(Date::DAY) == 1)
		{
			bestTasks.push_back(choseBestTask(std::make_shared<StartupBehavior>()));
		}

		Goals::TSubgoal bestTask = choseBestTask(bestTasks);

		if(bestTask->invalid())
		{
			logAi->trace("No goals found. Ending turn.");

			return;
		}

		if(bestTask->priority < MIN_PRIORITY)
		{
			logAi->trace("Goal %s has too low priority. It is not worth doing it. Ending turn.", bestTask->name());

			return;
		}

		logAi->debug("Trying to realize %s (value %2.3f)", bestTask->name(), bestTask->priority);

		try
		{
			if(bestTask->hero)
			{
				setActive(bestTask->hero.get(), bestTask->tile);
			}

			bestTask->accept(ai.get());
		}
		catch(goalFulfilledException &)
		{
			logAi->trace("Task %s completed", bestTask->name());
		}
		catch(std::exception & e)
		{
			logAi->debug("Failed to realize subgoal of type %s, I will stop.", bestTask->name());
			logAi->debug("The error message was: %s", e.what());

			return;
		}
	}
}
