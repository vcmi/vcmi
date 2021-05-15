#pragma once

#include "PriorityEvaluator.h"
#include "../Goals/AbstractGoal.h"
#include "../Behaviors/Behavior.h"

class Nullkiller
{
private:
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;

public:
	Nullkiller();
	void makeTurn();

private:
	Goals::TSubgoal choseBestTask(Behavior & behavior);
	Goals::TSubgoal choseBestTask(Goals::TGoalVec tasks);
};