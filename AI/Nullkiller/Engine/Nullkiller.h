#pragma once

#include "PriorityEvaluator.h"
#include "../Goals/AbstractGoal.h"
#include "../Behaviors/Behavior.h"

class Nullkiller
{
private:
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	HeroPtr activeHero;
	std::set<HeroPtr> lockedHeroes;

public:
	Nullkiller();
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero.h == hero; }
	void setActive(const HeroPtr & hero) { activeHero = hero; }
	void lockHero(const HeroPtr & hero) { lockedHeroes.insert(hero); }

private:
	void resetAiState();
	void updateAiState();
	Goals::TSubgoal choseBestTask(Behavior & behavior);
	Goals::TSubgoal choseBestTask(Goals::TGoalVec tasks);
};