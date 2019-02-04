#include "StdInc.h"
#include "Nullkiller.h"
#include "../VCAI.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Behaviors/RecruitHeroBehavior.h"
#include "../Goals/Invalid.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

Nullkiller::Nullkiller()
{
	priorityEvaluator.reset(new PriorityEvaluator());
}

Goals::TSubgoal Nullkiller::choseBestTask(Goals::TGoalVec tasks)
{
	Goals::TSubgoal bestTask = *vstd::maxElementByFun(tasks, [](Goals::TSubgoal goal) -> float
	{
		return goal->priority;
	});

	return bestTask;
}

Goals::TSubgoal Nullkiller::choseBestTask(Behavior & behavior)
{
	auto tasks = behavior.getTasks();

	if(tasks.empty())
	{
		logAi->debug("Behavior %s found no tasks", behavior.toString());

		return Goals::sptr(Goals::Invalid());
	}

	for(auto task : tasks)
	{
		task->setpriority(priorityEvaluator->evaluate(task));
	}

	auto task = choseBestTask(tasks);

	logAi->trace("Behavior %s found %s, priority %f", behavior.toString(), task->name(), task->priority);

	return task;
}

void Nullkiller::makeTurn()
{
	while(true)
	{
		Goals::TGoalVec bestTasks = {
			choseBestTask(CaptureObjectsBehavior()),
			choseBestTask(RecruitHeroBehavior())
		};

		Goals::TSubgoal bestTask = choseBestTask(bestTasks);

		if(bestTask->invalid())
		{
			logAi->trace("No goals found. Ending turn.");

			return;
		}

		logAi->debug("Trying to realize %s (value %2.3f)", bestTask->name(), bestTask->priority);

		try
		{
			bestTask->accept(ai.get());
		}
		catch(goalFulfilledException &)
		{
			logAi->trace(bestTask->completeMessage());
		}
		catch(std::exception & e)
		{
			logAi->debug("Failed to realize subgoal of type %s, I will stop.", bestTask->name());
			logAi->debug("The error message was: %s", e.what());

			return;
		}
	}
}