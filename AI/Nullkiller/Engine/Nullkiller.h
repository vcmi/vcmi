#pragma once

#include "PriorityEvaluator.h"
#include "DangerHitMapAnalyzer.h"
#include "../Goals/AbstractGoal.h"
#include "../Behaviors/Behavior.h"

class Nullkiller
{
private:
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	HeroPtr activeHero;
	std::set<HeroPtr> lockedHeroes;

public:
	std::unique_ptr<DangerHitMapAnalyzer> dangerHitMap;

	Nullkiller();
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero.h == hero; }
	void setActive(const HeroPtr & hero) { activeHero = hero; }
	void lockHero(const HeroPtr & hero) { lockedHeroes.insert(hero); }
	void unlockHero(const HeroPtr & hero) { lockedHeroes.erase(hero); }

private:
	void resetAiState();
	void updateAiState();
	Goals::TSubgoal choseBestTask(std::shared_ptr<Behavior> behavior) const;
	Goals::TSubgoal choseBestTask(Goals::TGoalVec & tasks) const;
};