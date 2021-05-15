#pragma once

#include "PriorityEvaluator.h"
#include "../Goals/AbstractGoal.h"
#include "../Behaviors/Behavior.h"

class Nullkiller
{
private:
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	HeroPtr activeHero;

public:
	Nullkiller();
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero.h == hero; }

private:
	Goals::TSubgoal choseBestTask(Behavior & behavior);
	Goals::TSubgoal choseBestTask(Goals::TGoalVec tasks);
};