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

#include "../../../lib/CPlayerState.h"
#include "../../lib/StartInfo.h"
#include "../../lib/pathfinder/PathfinderCache.h"
#include "../../lib/pathfinder/PathfinderOptions.h"
#include "../AIGateway.h"
#include "../Behaviors/BuildingBehavior.h"
#include "../Behaviors/BuyArmyBehavior.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Behaviors/ClusterBehavior.h"
#include "../Behaviors/DefenceBehavior.h"
#include "../Behaviors/ExplorationBehavior.h"
#include "../Behaviors/GatherArmyBehavior.h"
#include "../Behaviors/RecruitHeroBehavior.h"
#include "../Behaviors/StayAtTownBehavior.h"
#include "../Goals/Invalid.h"
#include "Goals/RecruitHero.h"
#include "ResourceTrader.h"
#include <boost/range/algorithm/sort.hpp>

namespace NK2AI
{

using namespace Goals;

// while we play vcmieagles graph can be shared
std::unique_ptr<ObjectGraph> Nullkiller::baseGraph;

Nullkiller::Nullkiller()
	: activeHero(nullptr)
	, scanDepth(ScanDepth::MAIN_FULL)
	, useHeroChain(true)
	, memory(std::make_unique<AIMemory>())
{

}

Nullkiller::~Nullkiller() = default;

bool canUseOpenMap(const std::shared_ptr<CCallback>& cb, const PlayerColor playerID)
{
	if(!cb->getStartInfo()->extraOptionsInfo.cheatsAllowed)
	{
		return false;
	}

	const TeamState * team = cb->getPlayerTeam(playerID);

	const auto hasHumanInTeam = vstd::contains_if(
		team->players,
		[cb](const PlayerColor teamMateID) -> bool
		{
			return cb->getPlayerState(teamMateID)->isHuman();
		}
	);

	return !hasHumanInTeam;
}

void Nullkiller::init(const std::shared_ptr<CCallback> & cbInput, AIGateway * aiGwInput)
{
	cc = cbInput;
	aiGw = aiGwInput;
	playerID = aiGwInput->playerID;

	settings = std::make_unique<Settings>(cc->getStartInfo()->difficulty);

	PathfinderOptions pathfinderOptions(*cc);
	pathfinderOptions.useTeleportTwoWay = true;
	pathfinderOptions.useTeleportOneWay = settings->isOneWayMonolithUsageAllowed();
	pathfinderOptions.useTeleportOneWayRandom = settings->isOneWayMonolithUsageAllowed();

	pathfinderCache = std::make_unique<PathfinderCache>(cc.get(), pathfinderOptions);

	if(canUseOpenMap(cc, playerID))
	{
		useObjectGraph = settings->isObjectGraphAllowed();
		openMap = settings->isOpenMap() || useObjectGraph;
	}
	else
	{
		useObjectGraph = false;
		openMap = false;
	}

	baseGraph.reset();
	priorityEvaluator.reset(new PriorityEvaluator(this));
	priorityEvaluators.reset(
		new SharedPool<PriorityEvaluator>(
			[&]()->std::unique_ptr<PriorityEvaluator>
			{
				return std::make_unique<PriorityEvaluator>(this);
			}));

	dangerHitMap.reset(new DangerHitMapAnalyzer(this));
	buildAnalyzer.reset(new BuildAnalyzer(this));
	objectClusterizer.reset(new ObjectClusterizer(this));
	dangerEvaluator.reset(new FuzzyHelper(this));
	pathfinder.reset(new AIPathfinder(this));
	armyManager.reset(new ArmyManager(cc.get(), this));
	heroManager.reset(new HeroManager(cc.get(), this));
	decomposer.reset(new DeepDecomposer(this));
	armyFormation.reset(new ArmyFormation(cc, this));
}

TaskPlanItem::TaskPlanItem(const TSubgoal & task)
	: affectedObjects(task->asTask()->getAffectedObjects())
	, task(task)
{
}

Goals::TTaskVec TaskPlan::getTasks() const
{
	Goals::TTaskVec result;

	for(auto & item : tasks)
	{
		result.push_back(taskptr(*item.task));
	}

	vstd::removeDuplicates(result);

	return result;
}

void TaskPlan::mergeAndFilter(const TSubgoal & task)
{
	TGoalVec blockers;

	if (task->asTask()->priority <= 0)
		return;

	for(auto & item : tasks)
	{
		for(auto objid : item.affectedObjects)
		{
			if(task == item.task || task->asTask()->isObjectAffected(objid) || (task->asTask()->getHero() != nullptr && task->asTask()->getHero() == item.task->asTask()->getHero()))
			{
				if(item.task->asTask()->priority >= task->asTask()->priority)
					return;

				blockers.push_back(item.task);
				break;
			}
		}
	}

	vstd::erase_if(tasks, [&](const TaskPlanItem & item)
		{
			return vstd::contains(blockers, item.task);
		});

	tasks.emplace_back(task);
}

Goals::TTask Nullkiller::choseBestTask(Goals::TGoalVec & tasks) const
{
	if(tasks.empty())
	{
		return taskptr(Invalid());
	}

	for(const TSubgoal & task : tasks)
	{
		if(task->asTask()->priority <= 0)
			task->asTask()->priority = priorityEvaluator->evaluate(task);
	}

	auto bestTask = *vstd::maxElementByFun(tasks, [](const Goals::TSubgoal& task) -> float
		{
			return task->asTask()->priority;
		});

	return taskptr(*bestTask);
}

Goals::TTaskVec Nullkiller::buildPlanAndFilter(TGoalVec & tasks, int priorityTier) const
{
	TaskPlan taskPlan;

	tbb::parallel_for(
		tbb::blocked_range<size_t>(0, tasks.size()),
		[this, &tasks, priorityTier](const tbb::blocked_range<size_t> & r)
		{
			const auto evaluator = this->priorityEvaluators->acquire();
			for(size_t i = r.begin(); i != r.end(); i++)
			{
				const auto & task = tasks[i];
				if(task->asTask()->priority <= 0 || priorityTier != PriorityEvaluator::PriorityTier::BUILDINGS)
					task->asTask()->priority = evaluator->evaluate(task, priorityTier);
			}
		}
	);

	boost::range::sort(
		tasks,
		[](const TSubgoal & g1, const TSubgoal & g2) -> bool
		{
			return g2->asTask()->priority < g1->asTask()->priority;
		}
	);

	for(const TSubgoal & task : tasks)
	{
		taskPlan.mergeAndFilter(task);
	}

	return taskPlan.getTasks();
}

void Nullkiller::decompose(Goals::TGoalVec & results, const Goals::TSubgoal& behavior, int decompositionMaxDepth) const
{
	makingTurnInterruption.interruptionPoint();
	logAi->debug("Decomposing behavior %s", behavior->toString());
	const auto start = std::chrono::high_resolution_clock::now();
	decomposer->decompose(results, behavior, decompositionMaxDepth);

	makingTurnInterruption.interruptionPoint();
	logAi->debug("Decomposing behavior %s done in %ld", behavior->toString(), timeElapsed(start));
}

void Nullkiller::resetState()
{
	std::unique_lock lockGuard(aiStateMutex);

	lockedResources = TResources();
	scanDepth = ScanDepth::MAIN_FULL;
	lockedHeroes.clear();
	dangerHitMap->resetHitmap();
	useHeroChain = true;
	objectClusterizer->reset();

	if(!baseGraph && isObjectGraphAllowed())
	{
		baseGraph = std::make_unique<ObjectGraph>();
		baseGraph->updateGraph(this);
	}
}

void Nullkiller::invalidatePathfinderData()
{
	pathfinderInvalidated = true;
}

void Nullkiller::updateState()
{
	const auto start = std::chrono::high_resolution_clock::now();
	auto startMethod = start;
	std::map<std::string, uint64_t> methodToElapsedMs = {};
	logAi->trace("PERFORMANCE: NK2 AI updateState started");

	makingTurnInterruption.interruptionPoint();
	std::unique_lock lockGuard(aiStateMutex);

	activeHero = nullptr;
	setTargetObject(-1);
	decomposer->reset();

	buildAnalyzer->update();
	methodToElapsedMs.emplace("buildAnalyzer->update", timeElapsed(startMethod));

	if(!pathfinderInvalidated && dangerHitMap->isHitMapUpToDate() && dangerHitMap->isTileOwnersUpToDate())
		logAi->trace("Skipping full state regeneration - up to date");
	else
	{
		startMethod = std::chrono::high_resolution_clock::now();
		memory->removeInvisibleOrDeletedObjects(*cc);
		methodToElapsedMs.emplace("memory->removeInvisibleOrDeletedObjects(*cc)", timeElapsed(startMethod));
		startMethod = std::chrono::high_resolution_clock::now();
		dangerHitMap->updateHitMap();
		methodToElapsedMs.emplace("dangerHitMap->updateHitMap()", timeElapsed(startMethod));
		startMethod = std::chrono::high_resolution_clock::now();
		dangerHitMap->calculateTileOwners();
		methodToElapsedMs.emplace("dangerHitMap->calculateTileOwners()", timeElapsed(startMethod));

		startMethod = std::chrono::high_resolution_clock::now();
		makingTurnInterruption.interruptionPoint();
		heroManager->update();
		methodToElapsedMs.emplace("heroManager->update()", timeElapsed(startMethod));

		logAi->trace("Updating paths");
		PathfinderSettings cfg;
		cfg.useHeroChain = useHeroChain;
		cfg.allowBypassObjects = true;

		if(scanDepth == ScanDepth::SMALL || isObjectGraphAllowed())
			cfg.mainTurnDistanceLimit = settings->getMainHeroTurnDistanceLimit();
		if(scanDepth != ScanDepth::ALL_FULL || isObjectGraphAllowed())
			cfg.scoutTurnDistanceLimit = settings->getScoutHeroTurnDistanceLimit();

		makingTurnInterruption.interruptionPoint();
		startMethod = std::chrono::high_resolution_clock::now();
		const auto heroes = getHeroesForPathfinding();
		methodToElapsedMs.emplace("getHeroesForPathfinding()", timeElapsed(startMethod));
		startMethod = std::chrono::high_resolution_clock::now();
		pathfinder->updatePaths(heroes, cfg);
		methodToElapsedMs.emplace("pathfinder->updatePaths(heroes, cfg)", timeElapsed(startMethod));

		if(isObjectGraphAllowed())
		{
			startMethod = std::chrono::high_resolution_clock::now();
			pathfinder->updateGraphs(
				heroes,
				scanDepth == ScanDepth::SMALL ? PathfinderSettings::MaxTurnDistanceLimit : 10, // TODO: Mircea: Move to constant
				scanDepth == ScanDepth::ALL_FULL ? PathfinderSettings::MaxTurnDistanceLimit : 3); // TODO: Mircea: Move to constant
			methodToElapsedMs.emplace("pathfinder->updateGraphs", timeElapsed(startMethod));
		}

		makingTurnInterruption.interruptionPoint();
		startMethod = std::chrono::high_resolution_clock::now();
		objectClusterizer->clusterize();
		methodToElapsedMs.emplace("objectClusterizer->clusterize()", timeElapsed(startMethod));
		pathfinderInvalidated = false;
	}

	startMethod = std::chrono::high_resolution_clock::now();
	armyManager->update();
	methodToElapsedMs.emplace("armyManager->update()", timeElapsed(startMethod));

	if(const auto timeElapsedMs = timeElapsed(start); timeElapsedMs > 250)
	{
		logAi->warn("PERFORMANCE: NK2 updateState took %ld ms", timeElapsedMs);

#if NK2AI_TRACE_LEVEL >= 1
		for(const auto & [name, elapsedMs] : methodToElapsedMs)
		{
			if(elapsedMs > 25)
				logAi->warn("PERFORMANCE: NK2 updateState %s took %ld ms", name, elapsedMs);
		}
#endif
	}
	else
	{
		logAi->info("PERFORMANCE: NK2 updateState took %ld ms", timeElapsedMs);
	}
}

bool Nullkiller::isHeroLocked(const CGHeroInstance * hero) const
{
	return getHeroLockedReason(hero) != HeroLockedReason::NOT_LOCKED;
}

bool Nullkiller::arePathHeroesLocked(const AIPath & path) const
{
	if(getHeroLockedReason(path.targetHero) == HeroLockedReason::STARTUP)
	{
#if NK2AI_TRACE_LEVEL >= 1
		logAi->trace("Hero %s is locked by STARTUP. Discarding %s", path.targetHero->getObjectName(), path.toString());
#endif
		return true;
	}

	for(const auto & node : path.nodes)
	{
		auto lockReason = getHeroLockedReason(node.targetHero);

		if(lockReason != HeroLockedReason::NOT_LOCKED)
		{
#if NK2AI_TRACE_LEVEL >= 1
			logAi->trace("Hero %s is locked by %d. Discarding %s", path.targetHero->getObjectName(), (int)lockReason,  path.toString());
#endif
			return true;
		}
	}

	return false;
}

HeroLockedReason Nullkiller::getHeroLockedReason(const CGHeroInstance * hero) const
{
	const auto found = lockedHeroes.find(hero);
	return found != lockedHeroes.end() ? found->second : HeroLockedReason::NOT_LOCKED;
}

void Nullkiller::makeTurn()
{
	std::lock_guard<std::mutex> sharedStorageLock(AISharedStorage::locker);
	pathfinderTurnStorageMisses.store(0);
	const int MAX_DEPTH = 10;
	resetState();
	Goals::TGoalVec tasks;
	tracePlayerStatus(true);

	for(int pass = 1; pass <= settings->getMaxPass() && cc->getPlayerStatus(playerID) == EPlayerStatus::INGAME; pass++)
	{
		if (pathfinderTurnStorageMisses.load() != 0)
		{
			logAi->warn("Nullkiller::makeTurn AINodeStorage had %d nodeAllocationFailures in previous pass", pathfinderTurnStorageMisses.load());
			pathfinderTurnStorageMisses.store(0);
		}

		if (!updateStateAndExecutePriorityPass(tasks, pass))
			return;

		tasks.clear();
		decompose(tasks, sptr(CaptureObjectsBehavior()), 1);
		decompose(tasks, sptr(ClusterBehavior()), MAX_DEPTH);
		decompose(tasks, sptr(DefenceBehavior()), MAX_DEPTH);
		decompose(tasks, sptr(GatherArmyBehavior()), MAX_DEPTH);
		// decompose(tasks, sptr(StayAtTownBehavior()), MAX_DEPTH);

		if(!isOpenMap())
			decompose(tasks, sptr(ExplorationBehavior()), MAX_DEPTH);

		TTaskVec selectedTasks;
		int prioOfTask = 0;
		for(int prio = PriorityEvaluator::PriorityTier::INSTAKILL; prio <= PriorityEvaluator::PriorityTier::MAX_PRIORITY_TIER; ++prio)
		{
			prioOfTask = prio;
			selectedTasks = buildPlanAndFilter(tasks, prio);
			if (!selectedTasks.empty() || settings->isUseFuzzy())
			{
				// Activate for deep debugging, otherwise too noisy even for trace level 2
// #if NK2AI_TRACE_LEVEL >= 2
// 				for(auto t : tasks)
// 				{
// 					logAi->info("task of prio %d: %s, prio: %f", prio, t->toString(), t->asTask()->priority);
// 				}
// 				for(auto t : selectedTasks)
// 				{
// 					logAi->info("selected task of prio %d: %s, prio: %f", prio, t->toString(), t->priority);
// 				}
// #endif
				break;
			}
		}

		boost::range::sort(selectedTasks, [](const TTask& a, const TTask& b)
		{
			return a->priority > b->priority;
		});

		if(selectedTasks.empty())
		{
			selectedTasks.push_back(taskptr(Goals::Invalid()));
		}

		bool hasAnySuccess = false;
		for(const auto& selectedTask : selectedTasks)
		{
			if(cc->getPlayerStatus(playerID) != EPlayerStatus::INGAME)
				return;

			if(!areAffectedObjectsPresent(selectedTask))
			{
				logAi->debug("Affected object not found. Canceling task.");
				continue;
			}

			std::string taskDescription = selectedTask->toString();
			HeroRole heroRole = getTaskRole(selectedTask);

			if(heroRole != HeroRole::MAIN || selectedTask->getHeroExchangeCount() <= 1)
				useHeroChain = false;

			// TODO: Mircea: AI suffers from lack of capability, no point to activate SMALL, to test more before deleting the code entirely
			// old-todo: better to check turn distance here instead of priority
			// if((heroRole != HeroRole::MAIN || selectedTask->priority < SMALL_SCAN_MIN_PRIORITY)
			// 	&& scanDepth == ScanDepth::MAIN_FULL)
			// {
			// 	useHeroChain = false;
			// 	scanDepth = ScanDepth::SMALL;
			//
			// 	logAi->trace(
			// 		"Goal %s has low priority %f so decreasing scan depth to gain performance.",
			// 		taskDescription,
			// 		selectedTask->priority);
			// }

			if((settings->isUseFuzzy() && selectedTask->priority < MIN_PRIORITY) || (!settings->isUseFuzzy() && selectedTask->priority <= 0))
			{
				auto heroes = cc->getHeroesInfo();
				const auto hasMp = vstd::contains_if(heroes, [](const CGHeroInstance * h) -> bool
					{
						return h->movementPointsRemaining() > 100;
					});

				if(hasMp && scanDepth != ScanDepth::ALL_FULL)
				{
					logAi->info(
						"Pass %d: Heroes can still move but goal %s has too low priority %f. Increasing to ScanDepth::ALL_FULL",
						taskDescription,
						selectedTask->priority);

					scanDepth = ScanDepth::ALL_FULL;
					useHeroChain = false;
					hasAnySuccess = true;
					break;
				}

				logAi->trace("Pass %d: Goal %s has too low priority. It is not worth doing it.", pass, taskDescription);
				continue;
			}

			logAi->info("Pass %d: Performing task (prioOfTask %d) %s with prio: %d", pass, prioOfTask, selectedTask->toString(), selectedTask->priority);

			if(HeroPtr heroPtr(selectedTask->getHero(), cc.get()); selectedTask->getHero() && !heroPtr.isVerified(false))
			{
				logAi->error("Nullkiller::makeTurn Skipping pass due to unverified hero: %s", heroPtr.nameOrDefault());
			}
			else
			{
				if(!executeTask(selectedTask))
				{
					if(hasAnySuccess)
						break;
					return;
				}
				hasAnySuccess = true;
			}
		}

		hasAnySuccess |= ResourceTrader::trade(*buildAnalyzer, *cc, getFreeResources());
		if(!hasAnySuccess)
		{
			logAi->trace("Nothing was done this turn pass. Ending turn.");
			tracePlayerStatus(false);
			return;
		}

		for(const auto * heroInfo : cc->getHeroesInfo())
			AIGateway::pickBestArtifacts(cc, heroInfo);

		if(pass == settings->getMaxPass())
			logAi->warn("MaxPass reached. Terminating AI turn.");
	}
}

bool Nullkiller::updateStateAndExecutePriorityPass(Goals::TGoalVec & tempResults, const int passIndex)
{
	updateState();

	Goals::TTask bestPrioPassTask = taskptr(Goals::Invalid());
	for(int i = 1; i <= settings->getMaxPriorityPass() && cc->getPlayerStatus(playerID) == EPlayerStatus::INGAME; i++)
	{
		tempResults.clear();

		// TODO: Mircea: Should merge recruiting from DefenseBehavior
		decompose(tempResults, sptr(RecruitHeroBehavior()), 1);
		// TODO: Mircea: Should merge recruiting from DefenseBehavior
		decompose(tempResults, sptr(BuyArmyBehavior()), 1);
		decompose(tempResults, sptr(BuildingBehavior()), 1);

		bestPrioPassTask = choseBestTask(tempResults);

		if(bestPrioPassTask->priority > 0)
		{
			logAi->info("Pass %d: priorityPass %d: Performing task %s with prio: %d", passIndex, i, bestPrioPassTask->toString(), bestPrioPassTask->priority);

			const bool isRecruitHeroGoal = dynamic_cast<RecruitHero*>(bestPrioPassTask.get()) != nullptr;
			HeroPtr heroPtr(bestPrioPassTask->getHero(), cc.get());
			if(!isRecruitHeroGoal && bestPrioPassTask->getHero() && !heroPtr.isVerified(false))
			{
				logAi->error("Nullkiller::updateStateAndExecutePriorityPass Skipping priorityPass due to unverified hero: %s", heroPtr.nameOrDefault());
			}
			else if(!executeTask(bestPrioPassTask))
			{
				logAi->warn("Task failed to execute");
				return false;
			}

			updateState();
		}
		else
		{
			break;
		}

		if(i == settings->getMaxPriorityPass())
		{
			logAi->warn("MaxPriorityPass reached. Terminating priorityPass loop.");
		}
	}
	return true;
}

bool Nullkiller::areAffectedObjectsPresent(const Goals::TTask & task) const
{
	auto affectedObjs = task->getAffectedObjects();

	for(auto oid : affectedObjs)
	{
		if(!cc->getObj(oid, false))
			return false;
	}

	return true;
}

HeroRole Nullkiller::getTaskRole(const Goals::TTask & task) const
{
	HeroPtr heroPtr(task->getHero(), cc.get());
	HeroRole heroRole = HeroRole::MAIN;

	if(heroPtr.isVerified())
		heroRole = heroManager->getHeroRoleOrDefault(heroPtr);

	return heroRole;
}

bool Nullkiller::executeTask(const Goals::TTask & task) const
{
	auto start = std::chrono::high_resolution_clock::now();
	std::string taskDescr = task->toString();

	makingTurnInterruption.interruptionPoint();
	logAi->debug("Trying to realize %s (value %2.3f)", taskDescr, task->priority);

	try
	{
		task->accept(aiGw);
		logAi->trace("Task %s completed in %lld", taskDescr, timeElapsed(start));
	}
	catch(goalFulfilledException &)
	{
		logAi->trace("Task %s completed in %lld", taskDescr, timeElapsed(start));
	}
	catch(cannotFulfillGoalException & e)
	{
		logAi->error("Failed to realize subgoal of type %s, I will stop.", taskDescr);
		logAi->error("The error message was: %s", e.what());
		return false;
	}

	return true;
}

TResources Nullkiller::getFreeResources() const
{
	auto freeRes = cc->getResourceAmount() - lockedResources;
	freeRes.positive();
	return freeRes;
}

void Nullkiller::lockResources(const TResources & res)
{
	lockedResources += res;
}

std::shared_ptr<const CPathsInfo> Nullkiller::getPathsInfo(const CGHeroInstance * h) const
{
	return pathfinderCache->getPathsInfo(h);
}

void Nullkiller::invalidatePaths()
{
	pathfinderCache->invalidatePaths();
}

void Nullkiller::tracePlayerStatus(bool beginning) const
{
#if NK2AI_TRACE_LEVEL >= 1
	float totalHeroesStrength = 0;
	int totalTownsLevel = 0;
	for (const auto *heroInfo : cc->getHeroesInfo())
	{
		totalHeroesStrength += heroInfo->getTotalStrength();
	}
	for (const auto *townInfo : cc->getTownsInfo())
	{
		totalTownsLevel += townInfo->getTownLevel();
	}

	const auto *firstWord = beginning ? "Beginning:" : "End:";
	logAi->info("%s totalHeroesStrength: %f, totalTownsLevel: %d, resources: %s", firstWord, totalHeroesStrength, totalTownsLevel, cc->getResourceAmount().toString());
#endif
}

std::map<const CGHeroInstance *, HeroRole> Nullkiller::getHeroesForPathfinding() const
{
	std::map<const CGHeroInstance *, HeroRole> activeHeroes;
	for(auto hero : cc->getHeroesInfo())
	{
		if(getHeroLockedReason(hero) == HeroLockedReason::DEFENCE)
			continue;

		activeHeroes[hero] = heroManager->getHeroRoleOrDefaultInefficient(hero);
	}
	return activeHeroes;
}

}
