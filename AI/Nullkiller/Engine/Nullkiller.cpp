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
#include "../Goals/Invalid.h"
#include "../Goals/Composition.h"

namespace NKAI
{

using namespace Goals;

#if NKAI_TRACE_LEVEL >= 1
#define MAXPASS 1000000
#else
#define MAXPASS 30
#endif

Nullkiller::Nullkiller()
{
	memory.reset(new AIMemory());
}

void Nullkiller::init(std::shared_ptr<CCallback> cb, PlayerColor playerID)
{
	this->cb = cb;
	this->playerID = playerID;

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
	decomposer.reset(new DeepDecomposer());
	armyFormation.reset(new ArmyFormation(cb, this));
}

Goals::TTask Nullkiller::choseBestTask(Goals::TTaskVec & tasks) const
{
	Goals::TTask bestTask = *vstd::maxElementByFun(tasks, [](Goals::TTask task) -> float{
		return task->priority;
	});

	return bestTask;
}

Goals::TTask Nullkiller::choseBestTask(Goals::TSubgoal behavior, int decompositionMaxDepth) const
{
	boost::this_thread::interruption_point();

	logAi->debug("Checking behavior %s", behavior->toString());

	auto start = std::chrono::high_resolution_clock::now();
	
	Goals::TGoalVec elementarGoals = decomposer->decompose(behavior, decompositionMaxDepth);
	Goals::TTaskVec tasks;

	boost::this_thread::interruption_point();
	
	for(auto goal : elementarGoals)
	{
		Goals::TTask task = Goals::taskptr(*goal);

		if(task->priority <= 0)
			task->priority = priorityEvaluator->evaluate(goal);

		tasks.push_back(task);
	}

	if(tasks.empty())
	{
		logAi->debug("Behavior %s found no tasks. Time taken %ld", behavior->toString(), timeElapsed(start));

		return Goals::taskptr(Goals::Invalid());
	}

	auto task = choseBestTask(tasks);

	logAi->debug(
		"Behavior %s returns %s, priority %f. Time taken %ld",
		behavior->toString(),
		task->toString(),
		task->priority,
		timeElapsed(start));

	return task;
}

void Nullkiller::resetAiState()
{
	lockedResources = TResources();
	scanDepth = ScanDepth::MAIN_FULL;
	playerID = ai->playerID;
	lockedHeroes.clear();
	dangerHitMap->reset();
	useHeroChain = true;
}

void Nullkiller::updateAiState(int pass, bool fast)
{
	boost::this_thread::interruption_point();

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

		if(scanDepth == ScanDepth::SMALL)
		{
			cfg.mainTurnDistanceLimit = MAIN_TURN_DISTANCE_LIMIT;
		}

		if(scanDepth != ScanDepth::ALL_FULL)
		{
			cfg.scoutTurnDistanceLimit = SCOUT_TURN_DISTANCE_LIMIT;
		}

		boost::this_thread::interruption_point();

		pathfinder->updatePaths(activeHeroes, cfg);

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

	for(int i = 1; i <= MAXPASS; i++)
	{
		updateAiState(i);

		Goals::TTask bestTask = taskptr(Goals::Invalid());

		for(;i <= MAXPASS; i++)
		{
			Goals::TTaskVec fastTasks = {
				choseBestTask(sptr(BuyArmyBehavior()), 1),
				choseBestTask(sptr(BuildingBehavior()), 1)
			};

			bestTask = choseBestTask(fastTasks);

			if(bestTask->priority >= FAST_TASK_MINIMAL_PRIORITY)
			{
				executeTask(bestTask);
				updateAiState(i, true);
			}
			else
			{
				break;
			}
		}

		Goals::TTaskVec bestTasks = {
			bestTask,
			choseBestTask(sptr(RecruitHeroBehavior()), 1),
			choseBestTask(sptr(CaptureObjectsBehavior()), 1),
			choseBestTask(sptr(ClusterBehavior()), MAX_DEPTH),
			choseBestTask(sptr(DefenceBehavior()), MAX_DEPTH),
			choseBestTask(sptr(GatherArmyBehavior()), MAX_DEPTH),
			choseBestTask(sptr(StayAtTownBehavior()), MAX_DEPTH)
		};

		if(cb->getDate(Date::DAY) == 1)
		{
			bestTasks.push_back(choseBestTask(sptr(StartupBehavior()), 1));
		}

		bestTask = choseBestTask(bestTasks);

		std::string taskDescription = bestTask->toString();
		HeroPtr hero = bestTask->getHero();
		HeroRole heroRole = HeroRole::MAIN;

		if(hero.validAndSet())
			heroRole = heroManager->getHeroRole(hero);

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
				continue;
			}

			logAi->trace("Goal %s has too low priority. It is not worth doing it. Ending turn.", taskDescription);

			return;
		}

		executeTask(bestTask);

		if(i == MAXPASS)
		{
			logAi->error("Goal %s exceeded maxpass. Terminating AI turn.", taskDescription);
		}
	}
}

void Nullkiller::executeTask(Goals::TTask task)
{
	auto start = std::chrono::high_resolution_clock::now();
	std::string taskDescr = task->toString();

	boost::this_thread::interruption_point();
	logAi->debug("Trying to realize %s (value %2.3f)", taskDescr, task->priority);

	try
	{
		task->accept(ai);
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

#if NKAI_TRACE_LEVEL == 0
		throw; // will be recatched and AI turn ended
#endif
	}
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
