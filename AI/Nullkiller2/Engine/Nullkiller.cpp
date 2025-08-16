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

#include <boost/range/algorithm/sort.hpp>
#include "../AIGateway.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Behaviors/RecruitHeroBehavior.h"
#include "../Behaviors/BuyArmyBehavior.h"
#include "../Behaviors/StartupBehavior.h"
#include "../Behaviors/DefenceBehavior.h"
#include "../Behaviors/BuildingBehavior.h"
#include "../Behaviors/GatherArmyBehavior.h"
#include "../Behaviors/ClusterBehavior.h"
#include "../Behaviors/StayAtTownBehavior.h"
#include "../Behaviors/ExplorationBehavior.h"
#include "../Goals/Invalid.h"
#include "../Goals/Composition.h"
#include "../../../lib/CPlayerState.h"
#include "../../lib/StartInfo.h"
#include "../../lib/pathfinder/PathfinderCache.h"
#include "../../lib/pathfinder/PathfinderOptions.h"

namespace NK2AI
{

using namespace Goals;

// while we play vcmieagles graph can be shared
std::unique_ptr<ObjectGraph> Nullkiller::baseGraph;

Nullkiller::Nullkiller()
	: activeHero(nullptr)
	, scanDepth(ScanDepth::MAIN_FULL)
	, useHeroChain(true)
	, pathfinderInvalidated(false)
	, memory(std::make_unique<AIMemory>())
{

}

Nullkiller::~Nullkiller() = default;

bool canUseOpenMap(std::shared_ptr<CCallback> cb, PlayerColor playerID)
{
	if(!cb->getStartInfo()->extraOptionsInfo.cheatsAllowed)
	{
		return false;
	}

	const TeamState * team = cb->getPlayerTeam(playerID);

	auto hasHumanInTeam = vstd::contains_if(team->players, [cb](PlayerColor teamMateID) -> bool
		{
			return cb->getPlayerState(teamMateID)->isHuman();
		});

	return !hasHumanInTeam;
}

void Nullkiller::init(std::shared_ptr<CCallback> cb, AIGateway * aiGw)
{
	this->cbc = cb;
	this->aiGw = aiGw;
	this->playerID = aiGw->playerID;

	settings = std::make_unique<Settings>(cb->getStartInfo()->difficulty);

	PathfinderOptions pathfinderOptions(*cb);

	pathfinderOptions.useTeleportTwoWay = true;
	pathfinderOptions.useTeleportOneWay = settings->isOneWayMonolithUsageAllowed();
	pathfinderOptions.useTeleportOneWayRandom = settings->isOneWayMonolithUsageAllowed();

	pathfinderCache = std::make_unique<PathfinderCache>(cb.get(), pathfinderOptions);

	if(canUseOpenMap(cb, playerID))
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
	pathfinder.reset(new AIPathfinder(cb.get(), this));
	armyManager.reset(new ArmyManager(cb.get(), this));
	heroManager.reset(new HeroManager(cb.get(), this));
	decomposer.reset(new DeepDecomposer(this));
	armyFormation.reset(new ArmyFormation(cb, this));
}

TaskPlanItem::TaskPlanItem(const TSubgoal & task)
	:task(task), affectedObjects(task->asTask()->getAffectedObjects())
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

void TaskPlan::merge(const TSubgoal & task)
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

	vstd::erase_if(tasks, [&](const TaskPlanItem & task2)
		{
			return vstd::contains(blockers, task2.task);
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

Goals::TTaskVec Nullkiller::buildPlan(TGoalVec & tasks, int priorityTier) const
{
	TaskPlan taskPlan;

	tbb::parallel_for(tbb::blocked_range<size_t>(0, tasks.size()), [this, &tasks, priorityTier](const tbb::blocked_range<size_t> & r)
		{
			auto evaluator = this->priorityEvaluators->acquire();

			for(size_t i = r.begin(); i != r.end(); i++)
			{
				const auto & task = tasks[i];
				if (task->asTask()->priority <= 0 || priorityTier != PriorityEvaluator::PriorityTier::BUILDINGS)
					task->asTask()->priority = evaluator->evaluate(task, priorityTier);
			}
		});

	boost::range::sort(tasks, [](const TSubgoal& g1, const TSubgoal& g2) -> bool
		{
			return g2->asTask()->priority < g1->asTask()->priority;
		});

	for(const TSubgoal & task : tasks)
	{
		taskPlan.merge(task);
	}

	return taskPlan.getTasks();
}

void Nullkiller::decompose(Goals::TGoalVec & results, const Goals::TSubgoal& behavior, int decompositionMaxDepth) const
{
	makingTurnInterrupption.interruptionPoint();
	logAi->debug("Decomposing behavior %s", behavior->toString());
	auto start = std::chrono::high_resolution_clock::now();
	decomposer->decompose(results, behavior, decompositionMaxDepth);

	makingTurnInterrupption.interruptionPoint();
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

void Nullkiller::updateState(bool partialUpdate)
{
	makingTurnInterrupption.interruptionPoint();
	std::unique_lock lockGuard(aiStateMutex);
	auto start = std::chrono::high_resolution_clock::now();

	activeHero = nullptr;
	setTargetObject(-1);
	decomposer->reset();

	buildAnalyzer->update();

	if (!pathfinderInvalidated)
		logAi->trace("Skipping paths regeneration - up to date");

	if(!partialUpdate && pathfinderInvalidated)
	{
		memory->removeInvisibleObjects(cbc.get());

		dangerHitMap->updateHitMap();
		dangerHitMap->calculateTileOwners();

		makingTurnInterrupption.interruptionPoint();

		heroManager->update();
		logAi->trace("Updating paths");

		std::map<const CGHeroInstance *, HeroRole> activeHeroes;

		for(auto hero : cbc->getHeroesInfo())
		{
			if(getHeroLockedReason(hero) == HeroLockedReason::DEFENCE)
				continue;

			activeHeroes[hero] = heroManager->getHeroRole(hero);
		}

		PathfinderSettings cfg;
		cfg.useHeroChain = useHeroChain;
		cfg.allowBypassObjects = true;

		if(scanDepth == ScanDepth::SMALL || isObjectGraphAllowed())
		{
			cfg.mainTurnDistanceLimit = settings->getMainHeroTurnDistanceLimit();
		}

		if(scanDepth != ScanDepth::ALL_FULL || isObjectGraphAllowed())
		{
			cfg.scoutTurnDistanceLimit =settings->getScoutHeroTurnDistanceLimit();
		}

		makingTurnInterrupption.interruptionPoint();

		pathfinder->updatePaths(activeHeroes, cfg);

		if(isObjectGraphAllowed())
		{
			pathfinder->updateGraphs(
				activeHeroes,
				scanDepth == ScanDepth::SMALL ? 255 : 10,
				scanDepth == ScanDepth::ALL_FULL ? 255 : 3);
		}

		makingTurnInterrupption.interruptionPoint();

		objectClusterizer->clusterize();

		pathfinderInvalidated = false;
	}

	armyManager->update();

	logAi->debug("AI state updated in %ld ms", timeElapsed(start));
}

bool Nullkiller::isHeroLocked(const CGHeroInstance * hero) const
{
	return getHeroLockedReason(hero) != HeroLockedReason::NOT_LOCKED;
}

bool Nullkiller::arePathHeroesLocked(const AIPath & path) const
{
	if(getHeroLockedReason(path.targetHero) == HeroLockedReason::STARTUP)
	{
#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("Hero %s is locked by STARTUP. Discarding %s", path.targetHero->getObjectName(), path.toString());
#endif
		return true;
	}

	for(auto & node : path.nodes)
	{
		auto lockReason = getHeroLockedReason(node.targetHero);

		if(lockReason != HeroLockedReason::NOT_LOCKED)
		{
#if NKAI_TRACE_LEVEL >= 1
			logAi->trace("Hero %s is locked by %d. Discarding %s", path.targetHero->getObjectName(), (int)lockReason,  path.toString());
#endif
			return true;
		}
	}

	return false;
}

HeroLockedReason Nullkiller::getHeroLockedReason(const CGHeroInstance * hero) const
{
	auto found = lockedHeroes.find(hero);

	return found != lockedHeroes.end() ? found->second : HeroLockedReason::NOT_LOCKED;
}

void Nullkiller::makeTurn()
{
	std::lock_guard<std::mutex> sharedStorageLock(AISharedStorage::locker);
	const int MAX_DEPTH = 10;
	resetState();
	Goals::TGoalVec tasks;
	tracePlayerStatus(true);

	for(int i = 1; i <= settings->getMaxPass() && cbc->getPlayerStatus(playerID) == EPlayerStatus::INGAME; i++)
	{
		updateState();

		if (!makeTurnHelperPriorityPass(tasks, i)) return;

		tasks.clear();
		decompose(tasks, sptr(CaptureObjectsBehavior()), 1);
		decompose(tasks, sptr(ClusterBehavior()), MAX_DEPTH);
		decompose(tasks, sptr(DefenceBehavior()), MAX_DEPTH);
		decompose(tasks, sptr(GatherArmyBehavior()), MAX_DEPTH);
		decompose(tasks, sptr(StayAtTownBehavior()), MAX_DEPTH);

		if(!isOpenMap())
			decompose(tasks, sptr(ExplorationBehavior()), MAX_DEPTH);

		TTaskVec selectedTasks;
		int prioOfTask = 0;
		for (int prio = PriorityEvaluator::PriorityTier::INSTAKILL; prio <= PriorityEvaluator::PriorityTier::MAX_PRIORITY_TIER; ++prio)
		{
			prioOfTask = prio;
			selectedTasks = buildPlan(tasks, prio);
			if (!selectedTasks.empty() || settings->isUseFuzzy())
				break;
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
			if(cbc->getPlayerStatus(playerID) != EPlayerStatus::INGAME)
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

			// TODO: better to check turn distance here instead of priority
			if((heroRole != HeroRole::MAIN || selectedTask->priority < SMALL_SCAN_MIN_PRIORITY)
				&& scanDepth == ScanDepth::MAIN_FULL)
			{
				useHeroChain = false;
				scanDepth = ScanDepth::SMALL;

				logAi->trace(
					"Goal %s has low priority %f so decreasing  scan depth to gain performance.",
					taskDescription,
					selectedTask->priority);
			}

			if((settings->isUseFuzzy() && selectedTask->priority < MIN_PRIORITY) || (!settings->isUseFuzzy() && selectedTask->priority <= 0))
			{
				auto heroes = cbc->getHeroesInfo();
				const auto hasMp = vstd::contains_if(heroes, [](const CGHeroInstance * h) -> bool
					{
						return h->movementPointsRemaining() > 100;
					});

				if(hasMp && scanDepth != ScanDepth::ALL_FULL)
				{
					logAi->trace(
						"Goal %s has too low priority %f so increasing scan depth to full.",
						taskDescription,
						selectedTask->priority);

					scanDepth = ScanDepth::ALL_FULL;
					useHeroChain = false;
					hasAnySuccess = true;
					break;
				}

				logAi->trace("Goal %s has too low priority. It is not worth doing it.", taskDescription);
				continue;
			}

#if NKAI_TRACE_LEVEL >= 1
			logAi->info("Pass %d: Performing prio %d task %s with prio: %d", i, prioOfTask, selectedTask->toString(), selectedTask->priority);
#endif

			if(!executeTask(selectedTask))
			{
				if(hasAnySuccess)
					break;
				return;
			}
			hasAnySuccess = true;
		}

		hasAnySuccess |= handleTrading();
		if(!hasAnySuccess)
		{
			logAi->trace("Nothing was done this turn. Ending turn.");
			tracePlayerStatus(false);
			return;
		}

		for (const auto *heroInfo : cbc->getHeroesInfo())
			AIGateway::pickBestArtifacts(cbc, heroInfo);

		if(i == settings->getMaxPass())
		{
			logAi->warn("MaxPass reached. Terminating AI turn.");
		}
	}
}

bool Nullkiller::makeTurnHelperPriorityPass(Goals::TGoalVec & tempResults, int passIndex)
{
	Goals::TTask bestPrioPassTask = taskptr(Goals::Invalid());
	for(int i = 1; i <= settings->getMaxPriorityPass() && cbc->getPlayerStatus(playerID) == EPlayerStatus::INGAME; i++)
	{
		tempResults.clear();

		decompose(tempResults, sptr(RecruitHeroBehavior()), 1);
		decompose(tempResults, sptr(BuyArmyBehavior()), 1);
		decompose(tempResults, sptr(BuildingBehavior()), 1);

		bestPrioPassTask = choseBestTask(tempResults);
		if(bestPrioPassTask->priority > 0)
		{
#if NKAI_TRACE_LEVEL >= 1
			logAi->info("Pass %d: Performing priorityPass %d task %s with prio: %d", passIndex, i, bestPrioPassTask->toString(), bestPrioPassTask->priority);
#endif

			if(!executeTask(bestPrioPassTask))
				return false;

			// TODO: Inspect why it's ok to do a partial update if condition is true
			updateState(bestPrioPassTask->getHero() == nullptr);
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

bool Nullkiller::areAffectedObjectsPresent(Goals::TTask task) const
{
	auto affectedObjs = task->getAffectedObjects();

	for(auto oid : affectedObjs)
	{
		if(!cbc->getObj(oid, false))
			return false;
	}

	return true;
}

HeroRole Nullkiller::getTaskRole(Goals::TTask task) const
{
	HeroPtr hero = task->getHero();
	HeroRole heroRole = HeroRole::MAIN;

	if(hero.validAndSet())
		heroRole = heroManager->getHeroRole(hero);

	return heroRole;
}

bool Nullkiller::executeTask(Goals::TTask task)
{
	auto start = std::chrono::high_resolution_clock::now();
	std::string taskDescr = task->toString();

	makingTurnInterrupption.interruptionPoint();
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
	auto freeRes = cbc->getResourceAmount() - lockedResources;

	freeRes.positive();

	return freeRes;
}

void Nullkiller::lockResources(const TResources & res)
{
	lockedResources += res;
}

bool Nullkiller::handleTrading()
{
	bool haveTraded = false;
	bool shouldTryToTrade = true;
	ObjectInstanceID marketId;
	for (auto town : cbc->getTownsInfo())
	{
		if (town->hasBuiltSomeTradeBuilding())
		{
			marketId = town->id;
		}
	}
	if (!marketId.hasValue())
		return false;
	if (const CGObjectInstance* obj = cbc->getObj(marketId, false))
	{
		if (const auto* m = dynamic_cast<const IMarket*>(obj))
		{
			while (shouldTryToTrade)
			{
				shouldTryToTrade = false;
				buildAnalyzer->update();
				TResources required = buildAnalyzer->getTotalResourcesRequired();
				TResources income = buildAnalyzer->getDailyIncome();
				TResources available = cbc->getResourceAmount();
#if NKAI_TRACE_LEVEL >= 2
				logAi->debug("Available %s", available.toString());
				logAi->debug("Required  %s", required.toString());
#endif
				int mostWanted = -1;
				int mostExpendable = -1;
				float minRatio = std::numeric_limits<float>::max();
				float maxRatio = std::numeric_limits<float>::min();

				for (int i = 0; i < required.size(); ++i)
				{
					if (required[i] <= 0)
						continue;
					float ratio = static_cast<float>(available[i]) / required[i];

					if (ratio < minRatio) {
						minRatio = ratio;
						mostWanted = i;
					}
				}

				for (int i = 0; i < required.size(); ++i)
				{
					float ratio = available[i];
					if (required[i] > 0)
						ratio = static_cast<float>(available[i]) / required[i];
					else
						ratio = available[i];

					bool okToSell = false;

					if (i == GameResID::GOLD)
					{
						if (income[i] > 0 && !buildAnalyzer->isGoldPressureOverMax())
							okToSell = true;
					}
					else
					{
						if (required[i] <= 0 && income[i] > 0)
							okToSell = true;
					}

					if (ratio > maxRatio && okToSell) {
						maxRatio = ratio;
						mostExpendable = i;
					}
				}
#if NKAI_TRACE_LEVEL >= 2
				logAi->debug("mostExpendable: %d mostWanted: %d", mostExpendable, mostWanted);
#endif
				if (mostExpendable == mostWanted || mostWanted == -1 || mostExpendable == -1)
					return false;

				int toGive;
				int toGet;
				m->getOffer(mostExpendable, mostWanted, toGive, toGet, EMarketMode::RESOURCE_RESOURCE);
				//logAi->info("Offer is: I get %d of %s for %d of %s at %s", toGet, mostWanted, toGive, mostExpendable, obj->getObjectName());
				//TODO trade only as much as needed
				if (toGive && toGive <= available[mostExpendable]) //don't try to sell 0 resources
				{
					cbc->trade(m->getObjInstanceID(), EMarketMode::RESOURCE_RESOURCE, GameResID(mostExpendable), GameResID(mostWanted), toGive);
#if NKAI_TRACE_LEVEL >= 2
					logAi->info("Traded %d of %s for %d of %s at %s", toGive, mostExpendable, toGet, mostWanted, obj->getObjectName());
#endif
					haveTraded = true;
					shouldTryToTrade = true;
				}
			}
		}
	}
	return haveTraded;
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
#if NKAI_TRACE_LEVEL >= 1
	float totalHeroesStrength = 0;
	int totalTownsLevel = 0;
	for (const auto *heroInfo : cbc->getHeroesInfo())
	{
		totalHeroesStrength += heroInfo->getTotalStrength();
	}
	for (const auto *townInfo : cbc->getTownsInfo())
	{
		totalTownsLevel += townInfo->getTownLevel();
	}

	const auto *firstWord = beginning ? "Beginning:" : "End:";
	logAi->info("%s totalHeroesStrength: %f, totalTownsLevel: %d, resources: %s", firstWord, totalHeroesStrength, totalTownsLevel, cbc->getResourceAmount().toString());
#endif
}

}
