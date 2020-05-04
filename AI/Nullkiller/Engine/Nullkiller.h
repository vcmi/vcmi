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
#include "FuzzyHelper.h"
#include "AIMemory.h"
#include "../Analyzers/DangerHitMapAnalyzer.h"
#include "../Analyzers/BuildAnalyzer.h"
#include "../Analyzers/ArmyManager.h"
#include "../Analyzers/HeroManager.h"
#include "../Analyzers/ObjectClusterizer.h"
#include "../Goals/AbstractGoal.h"

const float MAX_GOLD_PEASURE = 0.3f;
const float MIN_PRIORITY = 0.01f;

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
	const CGHeroInstance * activeHero;
	int3 targetTile;
	std::map<const CGHeroInstance *, HeroLockedReason> lockedHeroes;

public:
	std::unique_ptr<DangerHitMapAnalyzer> dangerHitMap;
	std::unique_ptr<BuildAnalyzer> buildAnalyzer;
	std::unique_ptr<ObjectClusterizer> objectClusterizer;
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	std::unique_ptr<AIPathfinder> pathfinder;
	std::unique_ptr<HeroManager> heroManager;
	std::unique_ptr<ArmyManager> armyManager;
	std::unique_ptr<AIMemory> memory;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;
	PlayerColor playerID;
	std::shared_ptr<CCallback> cb;

	Nullkiller();
	void init(std::shared_ptr<CCallback> cb, PlayerColor playerID);
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero == hero; }
	bool isHeroLocked(const CGHeroInstance * hero) const;
	HeroPtr getActiveHero() { return activeHero; }
	HeroLockedReason getHeroLockedReason(const CGHeroInstance * hero) const;
	int3 getTargetTile() const { return targetTile; }
	void setActive(const CGHeroInstance * hero, int3 tile) { activeHero = hero; targetTile = tile; }
	void lockHero(const CGHeroInstance * hero, HeroLockedReason lockReason) { lockedHeroes[hero] = lockReason; }
	void unlockHero(const CGHeroInstance * hero) { lockedHeroes.erase(hero); }
	bool arePathHeroesLocked(const AIPath & path) const;

private:
	void resetAiState();
	void updateAiState();
	Goals::TTask choseBestTask(Goals::TSubgoal behavior) const;
	Goals::TTask choseBestTask(Goals::TTaskVec & tasks) const;
};
