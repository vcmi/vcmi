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
	ai->validateVisitableObjs();
	dangerHitMap->updateHitMap();

	// TODO: move to hero manager
	auto activeHeroes = ai->getMyHeroes();

	vstd::erase_if(activeHeroes, [this](const HeroPtr & hero) -> bool
	{
		auto lockedReason = getHeroLockedReason(hero.h);

		return lockedReason == HeroLockedReason::DEFENCE || lockedReason == HeroLockedReason::STARTUP;
	});

	ai->ah->updatePaths(activeHeroes, true);
	ai->ah->update();

	buildAnalyzer->update();
}

bool Nullkiller::arePathHeroesLocked(const AIPath & path) const
{
	for(auto & node : path.nodes)
	{
		if(isHeroLocked(node.targetHero))
			return true;
	}

	return false;
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
			choseBestTask(std::make_shared<BuildingBehavior>())
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

		logAi->debug("Trying to realize %s (value %2.3f)", bestTask->name(), bestTask->priority);

		try
		{
			activeHero = bestTask->hero.get();

			bestTask->accept(ai.get());
		}
		catch(goalFulfilledException &)
		{
			logAi->trace(bestTask->completeMessage());
		}
		/*catch(std::exception & e)
		{
			logAi->debug("Failed to realize subgoal of type %s, I will stop.", bestTask->name());
			logAi->debug("The error message was: %s", e.what());

			return;
		}*/
	}
}
