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

namespace NKAI
{

using namespace Goals;

// while we play vcmieagles graph can be shared
std::unique_ptr<ObjectGraph> Nullkiller::baseGraph;

Nullkiller::Nullkiller()
	:activeHero(nullptr), scanDepth(ScanDepth::MAIN_FULL), useHeroChain(true)
{
	memory = std::make_unique<AIMemory>();
	settings = std::make_unique<Settings>();

	useObjectGraph = settings->isObjectGraphAllowed();
	openMap = settings->isOpenMap() || useObjectGraph;
}

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

	if(hasHumanInTeam)
	{
		return false;
	}

	return cb->getStartInfo()->difficulty >= 3;
}

void Nullkiller::init(std::shared_ptr<CCallback> cb, AIGateway * gateway)
{
	this->cb = cb;
	this->gateway = gateway;
	
	playerID = gateway->playerID;

	if(openMap && !canUseOpenMap(cb, playerID))
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

TaskPlanItem::TaskPlanItem(TSubgoal task)
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

void TaskPlan::merge(TSubgoal task)
{
	TGoalVec blockers;

	for(auto & item : tasks)
	{
		for(auto objid : item.affectedObjects)
		{
			if(task == item.task || task->asTask()->isObjectAffected(objid))
			{
				if(item.task->asTask()->priority >= task->asTask()->priority)
					return;

				blockers.push_back(item.task);
				break;
			}
		}
	}

	vstd::erase_if(tasks, [&](const TaskPlanItem & task)
		{
			return vstd::contains(blockers, task.task);
		});

	tasks.emplace_back(task);
}

Goals::TTask Nullkiller::choseBestTask(Goals::TGoalVec & tasks) const
{
	if(tasks.empty())
	{
		return taskptr(Invalid());
	}

	for(TSubgoal & task : tasks)
	{
		if(task->asTask()->priority <= 0)
			task->asTask()->priority = priorityEvaluator->evaluate(task);
	}

	auto bestTask = *vstd::maxElementByFun(tasks, [](Goals::TSubgoal task) -> float
		{
			return task->asTask()->priority;
		});

	return taskptr(*bestTask);
}

Goals::TTaskVec Nullkiller::buildPlan(TGoalVec & tasks) const
{
	TaskPlan taskPlan;

	tbb::parallel_for(tbb::blocked_range<size_t>(0, tasks.size()), [this, &tasks](const tbb::blocked_range<size_t> & r)
		{
			auto evaluator = this->priorityEvaluators->acquire();

			for(size_t i = r.begin(); i != r.end(); i++)
			{
				auto task = tasks[i];

				if(task->asTask()->priority <= 0)
					task->asTask()->priority = evaluator->evaluate(task);
			}
		});

	std::sort(tasks.begin(), tasks.end(), [](TSubgoal g1, TSubgoal g2) -> bool
		{
			return g2->asTask()->priority < g1->asTask()->priority;
		});

	for(TSubgoal & task : tasks)
	{
		taskPlan.merge(task);
	}

	return taskPlan.getTasks();
}

void Nullkiller::decompose(Goals::TGoalVec & result, Goals::TSubgoal behavior, int decompositionMaxDepth) const
{
	boost::this_thread::interruption_point();

	logAi->debug("Checking behavior %s", behavior->toString());

	auto start = std::chrono::high_resolution_clock::now();
	
	decomposer->decompose(result, behavior, decompositionMaxDepth);

	boost::this_thread::interruption_point();

	logAi->debug(
		"Behavior %s. Time taken %ld",
		behavior->toString(),
		timeElapsed(start));
}

void Nullkiller::resetAiState()
{
	std::unique_lock lockGuard(aiStateMutex);

	lockedResources = TResources();
	scanDepth = ScanDepth::MAIN_FULL;
	lockedHeroes.clear();
	dangerHitMap->reset();
	useHeroChain = true;
	objectClusterizer->reset();

	if(!baseGraph && isObjectGraphAllowed())
	{
		baseGraph = std::make_unique<ObjectGraph>();
		baseGraph->updateGraph(this);
	}
}

void Nullkiller::updateAiState(int pass, bool fast)
{
	boost::this_thread::interruption_point();

	std::unique_lock lockGuard(aiStateMutex);

	auto start = std::chrono::high_resolution_clock::now();

	activeHero = nullptr;
	setTargetObject(-1);

	decomposer->reset();
	buildAnalyzer->update();

	if(!fast)
	{
		memory->removeInvisibleObjects(cb.get());

		dangerHitMap->updateHitMap();
		dangerHitMap->calculateTileOwners();

		boost::this_thread::interruption_point();

		heroManager->update();
		logAi->trace("Updating paths");

		std::map<const CGHeroInstance *, HeroRole> activeHeroes;

		for(auto hero : cb->getHeroesInfo())
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

		boost::this_thread::interruption_point();

		pathfinder->updatePaths(activeHeroes, cfg);

		if(isObjectGraphAllowed())
		{
			pathfinder->updateGraphs(
				activeHeroes,
				scanDepth == ScanDepth::SMALL ? 255 : 10,
				scanDepth == ScanDepth::ALL_FULL ? 255 : 3);
		}

		boost::this_thread::interruption_point();

		objectClusterizer->clusterize();
	}

	armyManager->update();

	logAi->debug("AI state updated in %ld", timeElapsed(start));
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
			logAi->trace("Hero %s is locked by STARTUP. Discarding %s", path.targetHero->getObjectName(), path.toString());
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
	boost::lock_guard<boost::mutex> sharedStorageLock(AISharedStorage::locker);

	const int MAX_DEPTH = 10;
	const float FAST_TASK_MINIMAL_PRIORITY = 0.7f;

	resetAiState();

	Goals::TGoalVec bestTasks;

	for(int i = 1; i <= settings->getMaxPass() && cb->getPlayerStatus(playerID) == EPlayerStatus::INGAME; i++)
	{
		auto start = std::chrono::high_resolution_clock::now();
		updateAiState(i);

		Goals::TTask bestTask = taskptr(Goals::Invalid());

		for(;i <= settings->getMaxPass(); i++)
		{
			bestTasks.clear();

			decompose(bestTasks, sptr(BuyArmyBehavior()), 1);
			decompose(bestTasks, sptr(BuildingBehavior()), 1);

			bestTask = choseBestTask(bestTasks);

			if(bestTask->priority >= FAST_TASK_MINIMAL_PRIORITY)
			{
				if(!executeTask(bestTask))
					return;

				updateAiState(i, true);
			}
			else
			{
				break;
			}
		}

		decompose(bestTasks, sptr(RecruitHeroBehavior()), 1);
		decompose(bestTasks, sptr(CaptureObjectsBehavior()), 1);
		decompose(bestTasks, sptr(ClusterBehavior()), MAX_DEPTH);
		decompose(bestTasks, sptr(DefenceBehavior()), MAX_DEPTH);
		decompose(bestTasks, sptr(GatherArmyBehavior()), MAX_DEPTH);
		decompose(bestTasks, sptr(StayAtTownBehavior()), MAX_DEPTH);

		if(!isOpenMap())
			decompose(bestTasks, sptr(ExplorationBehavior()), MAX_DEPTH);

		if(cb->getDate(Date::DAY) == 1 || heroManager->getHeroRoles().empty())
		{
			decompose(bestTasks, sptr(StartupBehavior()), 1);
		}

		auto selectedTasks = buildPlan(bestTasks);

		logAi->debug("Decision madel in %ld", timeElapsed(start));

		if(selectedTasks.empty())
		{
			selectedTasks.push_back(taskptr(Goals::Invalid()));
		}

		bool hasAnySuccess = false;

		for(auto bestTask : selectedTasks)
		{
			if(cb->getPlayerStatus(playerID) != EPlayerStatus::INGAME)
				return;

			if(!areAffectedObjectsPresent(bestTask))
			{
				logAi->debug("Affected object not found. Canceling task.");
				continue;
			}

			std::string taskDescription = bestTask->toString();
			HeroRole heroRole = getTaskRole(bestTask);

			if(heroRole != HeroRole::MAIN || bestTask->getHeroExchangeCount() <= 1)
				useHeroChain = false;

			// TODO: better to check turn distance here instead of priority
			if((heroRole != HeroRole::MAIN || bestTask->priority < SMALL_SCAN_MIN_PRIORITY)
				&& scanDepth == ScanDepth::MAIN_FULL)
			{
				useHeroChain = false;
				scanDepth = ScanDepth::SMALL;

				logAi->trace(
					"Goal %s has low priority %f so decreasing  scan depth to gain performance.",
					taskDescription,
					bestTask->priority);
			}

			if(bestTask->priority < MIN_PRIORITY)
			{
				auto heroes = cb->getHeroesInfo();
				auto hasMp = vstd::contains_if(heroes, [](const CGHeroInstance * h) -> bool
					{
						return h->movementPointsRemaining() > 100;
					});

				if(hasMp && scanDepth != ScanDepth::ALL_FULL)
				{
					logAi->trace(
						"Goal %s has too low priority %f so increasing scan depth to full.",
						taskDescription,
						bestTask->priority);

					scanDepth = ScanDepth::ALL_FULL;
					useHeroChain = false;
					hasAnySuccess = true;
					break;
				}

				logAi->trace("Goal %s has too low priority. It is not worth doing it.", taskDescription);

				continue;
			}

			if(!executeTask(bestTask))
			{
				if(hasAnySuccess)
					break;
				else
					return;
			}

			hasAnySuccess = true;
		}

		if(!hasAnySuccess)
		{
			logAi->trace("Nothing was done this turn. Ending turn.");
			return;
		}

		if(i == settings->getMaxPass())
		{
			logAi->warn("Maxpass exceeded. Terminating AI turn.");
		}
	}
}

bool Nullkiller::areAffectedObjectsPresent(Goals::TTask task) const
{
	auto affectedObjs = task->getAffectedObjects();

	for(auto oid : affectedObjs)
	{
		if(!cb->getObj(oid, false))
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

	boost::this_thread::interruption_point();
	logAi->debug("Trying to realize %s (value %2.3f)", taskDescr, task->priority);

	try
	{
		task->accept(gateway);
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
	auto freeRes = cb->getResourceAmount() - lockedResources;

	freeRes.positive();

	return freeRes;
}

void Nullkiller::lockResources(const TResources & res)
{
	lockedResources += res;
}

}
