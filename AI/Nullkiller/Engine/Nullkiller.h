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
#include "Settings.h"
#include "AIMemory.h"
#include "DeepDecomposer.h"
#include "../Analyzers/DangerHitMapAnalyzer.h"
#include "../Analyzers/BuildAnalyzer.h"
#include "../Analyzers/ArmyManager.h"
#include "../Analyzers/HeroManager.h"
#include "../Analyzers/ObjectClusterizer.h"
#include "../Helpers/ArmyFormation.h"

#include "../../../lib/ConditionalWait.h"

VCMI_LIB_NAMESPACE_BEGIN

class PathfinderCache;

VCMI_LIB_NAMESPACE_END

namespace NKAI
{

const float MIN_PRIORITY = 0.01f;
const float SMALL_SCAN_MIN_PRIORITY = 0.4f;

enum class HeroLockedReason
{
	NOT_LOCKED = 0,

	STARTUP = 1,

	DEFENCE = 2,

	HERO_CHAIN = 3
};

enum class ScanDepth
{
	MAIN_FULL = 0,

	SMALL = 1,

	ALL_FULL = 2
};

struct TaskPlanItem
{
	std::vector<ObjectInstanceID> affectedObjects;
	Goals::TSubgoal task;

	TaskPlanItem(Goals::TSubgoal goal);
};

class TaskPlan
{
private:
	std::vector<TaskPlanItem> tasks;

public:
	Goals::TTaskVec getTasks() const;
	void merge(Goals::TSubgoal task);
};

class Nullkiller
{
private:
	const CGHeroInstance * activeHero;
	int3 targetTile;
	ObjectInstanceID targetObject;
	std::map<const CGHeroInstance *, HeroLockedReason> lockedHeroes;
	std::unique_ptr<PathfinderCache> pathfinderCache;
	ScanDepth scanDepth;
	TResources lockedResources;
	bool useHeroChain;
	AIGateway * gateway;
	bool openMap;
	bool useObjectGraph;
	bool pathfinderInvalidated;

public:
	static std::unique_ptr<ObjectGraph> baseGraph;

	std::unique_ptr<DangerHitMapAnalyzer> dangerHitMap;
	std::unique_ptr<BuildAnalyzer> buildAnalyzer;
	std::unique_ptr<ObjectClusterizer> objectClusterizer;
	std::unique_ptr<PriorityEvaluator> priorityEvaluator;
	std::unique_ptr<SharedPool<PriorityEvaluator>> priorityEvaluators;
	std::unique_ptr<AIPathfinder> pathfinder;
	std::unique_ptr<HeroManager> heroManager;
	std::unique_ptr<ArmyManager> armyManager;
	std::unique_ptr<AIMemory> memory;
	std::unique_ptr<FuzzyHelper> dangerEvaluator;
	std::unique_ptr<DeepDecomposer> decomposer;
	std::unique_ptr<ArmyFormation> armyFormation;
	std::unique_ptr<Settings> settings;
	PlayerColor playerID;
	std::shared_ptr<CCallback> cb;
	std::mutex aiStateMutex;
	mutable ThreadInterruption makingTurnInterrupption;

	Nullkiller();
	~Nullkiller();
	void init(std::shared_ptr<CCallback> cb, AIGateway * gateway);
	void makeTurn();
	bool isActive(const CGHeroInstance * hero) const { return activeHero == hero; }
	bool isHeroLocked(const CGHeroInstance * hero) const;
	HeroPtr getActiveHero() { return activeHero; }
	HeroLockedReason getHeroLockedReason(const CGHeroInstance * hero) const;
	int3 getTargetTile() const { return targetTile; }
	ObjectInstanceID getTargetObject() const { return targetObject; }
	void setTargetObject(int objid) { targetObject = ObjectInstanceID(objid); }
	void setActive(const CGHeroInstance * hero, int3 tile) { activeHero = hero; targetTile = tile; }
	void lockHero(const CGHeroInstance * hero, HeroLockedReason lockReason) { lockedHeroes[hero] = lockReason; }
	void unlockHero(const CGHeroInstance * hero) { lockedHeroes.erase(hero); }
	bool arePathHeroesLocked(const AIPath & path) const;
	TResources getFreeResources() const;
	int32_t getFreeGold() const { return getFreeResources()[EGameResID::GOLD]; }
	void lockResources(const TResources & res);
	const TResources & getLockedResources() const { return lockedResources; }
	ScanDepth getScanDepth() const { return scanDepth; }
	bool isOpenMap() const { return openMap; }
	bool isObjectGraphAllowed() const { return useObjectGraph; }
	bool handleTrading();
	void invalidatePathfinderData();

	std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h) const;
	void invalidatePaths();

private:
	void resetAiState();
	void updateAiState(int pass, bool fast = false);
	void decompose(Goals::TGoalVec & result, Goals::TSubgoal behavior, int decompositionMaxDepth) const;
	Goals::TTask choseBestTask(Goals::TGoalVec & tasks) const;
	Goals::TTaskVec buildPlan(Goals::TGoalVec & tasks, int priorityTier) const;
	bool executeTask(Goals::TTask task);
	bool areAffectedObjectsPresent(Goals::TTask task) const;
	HeroRole getTaskRole(Goals::TTask task) const;
};

}
