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
#include "../Analyzers/DangerHitMapAnalyzer.h"
#include "../Analyzers/BuildAnalyzer.h"
#include "../Goals/AbstractGoal.h"
#include "../Behaviors/Behavior.h"

enum class HeroLockedReason
{
	NOT_LOCKED = 0,

	STARTUP = 1,

	DEFENCE = 2,

	HERO_CHAIN = 3
};

class Nullkiller
{
private:
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	const CGHeroInstance * activeHero;
	std::map<const CGHeroInstance *, HeroLockedReason> lockedHeroes;

public:
	std::unique_ptr<DangerHitMapAnalyzer> dangerHitMap;

	Nullkiller();
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero == hero; }
	bool isHeroLocked(const CGHeroInstance * hero) const { return vstd::contains(lockedHeroes, hero); }
	HeroLockedReason getHeroLockedReason(const CGHeroInstance * hero) const { return isHeroLocked(hero) ? lockedHeroes.at(hero) : HeroLockedReason::NOT_LOCKED; }
	void setActive(const CGHeroInstance * hero) { activeHero = hero; }
	void lockHero(const CGHeroInstance * hero, HeroLockedReason lockReason) { lockedHeroes[hero] = lockReason; }
	void unlockHero(const CGHeroInstance * hero) { lockedHeroes.erase(hero); }
	bool arePathHeroesLocked(const AIPath & path) const;

private:
	void resetAiState();
	void updateAiState();
	Goals::TSubgoal choseBestTask(std::shared_ptr<Behavior> behavior) const;
	Goals::TSubgoal choseBestTask(Goals::TGoalVec & tasks) const;
};
