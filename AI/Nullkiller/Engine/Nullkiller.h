/*
* Nullkiller.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "PriorityEvaluator.h"
#include "DangerHitMapAnalyzer.h"
#include "../Goals/AbstractGoal.h"
#include "../Behaviors/Behavior.h"

class Nullkiller
{
private:
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	const CGHeroInstance * activeHero;
	std::set<const CGHeroInstance *> lockedHeroes;

public:
	std::unique_ptr<DangerHitMapAnalyzer> dangerHitMap;

	Nullkiller();
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero == hero; }
	bool isHeroLocked(const CGHeroInstance * hero) const { return vstd::contains(lockedHeroes, hero); }
	void setActive(const CGHeroInstance * hero) { activeHero = hero; }
	void lockHero(const CGHeroInstance * hero) { lockedHeroes.insert(hero); }
	void unlockHero(const CGHeroInstance * hero) { lockedHeroes.erase(hero); }
	bool canMove(const CGHeroInstance * hero) { return hero->movement; }

private:
	void resetAiState();
	void updateAiState();
	Goals::TSubgoal choseBestTask(std::shared_ptr<Behavior> behavior) const;
	Goals::TSubgoal choseBestTask(Goals::TGoalVec & tasks) const;
};
