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
#include "DeepDecomposer.h"
#include "../AIGateway.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Behaviors/RecruitHeroBehavior.h"
#include "../Behaviors/BuyArmyBehavior.h"
#include "../Behaviors/StartupBehavior.h"
#include "../Behaviors/DefenceBehavior.h"
#include "../Behaviors/BuildingBehavior.h"
#include "../Behaviors/GatherArmyBehavior.h"
#include "../Behaviors/ClusterBehavior.h"
#include "../Goals/Invalid.h"
#include "../Goals/Composition.h"

namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

void DeepDecomposer::reset()
{
	decompositionCache.clear();
	goals.clear();
}

Goals::TGoalVec DeepDecomposer::decompose(TSubgoal behavior, int depthLimit)
{
	TGoalVec tasks;

	goals.clear();
	goals.resize(depthLimit);
	decompositionCache.resize(depthLimit);
	depth = 0;

	goals[0] = {behavior};

	while(goals[0].size())
	{
		bool fromCache;
		TSubgoal current = goals[depth].back();
		TGoalVec subgoals = decomposeCached(unwrapComposition(current), fromCache);

#if NKAI_TRACE_LEVEL >= 1
		logAi->trace("Decomposition level %d returned %d goals", depth, subgoals.size());
#endif

		if(depth < depthLimit - 1)
		{
			goals[depth + 1].clear();
		}

		for(TSubgoal subgoal : subgoals)
		{
			if(subgoal->invalid())
				continue;

			if(subgoal->isElementar())
			{
				// need to get rid of priority control in behaviors like Startup to avoid this check.
				// 0 - goals directly from behavior
				Goals::TSubgoal task = depth >= 1 ? aggregateGoals(0, subgoal) : subgoal;

#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Found task %s", task->toString());
#endif
				if(!isCompositionLoop(subgoal))
				{
					tasks.push_back(task);

					if(!fromCache)
					{
						addToCache(subgoal);
					}
				}
			}
			else if(depth < depthLimit - 1)
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Found abstract goal %s", subgoal->toString());
#endif
				if(!isCompositionLoop(subgoal))
				{
					// depth 0 gives reward, deepest goal - task to execute, all the rest is not important
					// so avoid details to reduce decomposition complexity but they can give some negative effect so
					auto goalToAdd = depth >= 1 ? unwrapComposition(subgoal) : subgoal; 

					if(!vstd::contains(goals[depth + 1], goalToAdd))
					{
						goals[depth + 1].push_back(subgoal);
					}
				}
			}
		}

		if(depth < depthLimit - 1 && goals[depth + 1].size())
		{
			depth++;
		}
		else
		{
			goals[depth].pop_back();

			while(depth > 0 && goals[depth].empty())
			{
				depth--;
				goals[depth].pop_back();
			}
		}
	}

	return tasks;
}

Goals::TSubgoal DeepDecomposer::aggregateGoals(int startDepth, TSubgoal last)
{
	Goals::Composition composition;

	for(int i = 0; i <= depth; i++)
	{
		composition.addNext(goals[i].back());
	}

	composition.addNext(last);

	return sptr(composition);
}

Goals::TSubgoal DeepDecomposer::unwrapComposition(Goals::TSubgoal goal)
{
	return goal->goalType == Goals::COMPOSITION ? goal->decompose().back() : goal;
}

bool isEquivalentGoals(TSubgoal goal1, TSubgoal goal2)
{
	if(goal1 == goal2) return true;

	if(goal1->goalType == Goals::CAPTURE_OBJECT && goal2->goalType == Goals::CAPTURE_OBJECT)
	{
		auto o1 = cb->getObj(ObjectInstanceID(goal1->objid));
		auto o2 = cb->getObj(ObjectInstanceID(goal2->objid));

		return o1->ID == Obj::SHIPYARD && o1->ID == o2->ID;
	}

	return false;
}

bool DeepDecomposer::isCompositionLoop(TSubgoal goal)
{
	auto goalsToTest = goal->goalType == Goals::COMPOSITION ? goal->decompose() : TGoalVec{goal};

	for(auto goalToTest : goalsToTest)
	{
		for(int i = depth; i >= 0; i--)
		{
			auto parent = unwrapComposition(goals[i].back());

			if(isEquivalentGoals(parent, goalToTest))
			{
				return true;
			}
		}
	}

	return false;
}

TGoalVec DeepDecomposer::decomposeCached(TSubgoal goal, bool & fromCache)
{
#if NKAI_TRACE_LEVEL >= 1	
	logAi->trace("Decomposing %s, level %s", goal->toString(), depth);
#endif

	if(goal->hasHash())
	{
		for(int i = 0; i <= depth; i++)
		{
			auto cached = decompositionCache[i].find(goal);

			if(cached != decompositionCache[i].end())
			{
#if NKAI_TRACE_LEVEL >= 1
				logAi->trace("Use decomposition cache for %s, level: %d", goal->toString(), depth);
#endif
				fromCache = true;

				return cached->second;
			}
		}

		decompositionCache[depth][goal] = {}; // if goal decomposition yields no goals we still need it in cache to not decompose again
	}

#if NKAI_TRACE_LEVEL >= 2	
	logAi->trace("Calling decompose on %s, level %s", goal->toString(), depth);
#endif

	fromCache = false;

	return goal->decompose();
}

void DeepDecomposer::addToCache(TSubgoal goal)
{
	bool trusted = true;

	for(int parentDepth = 1; parentDepth <= depth; parentDepth++)
	{
		TSubgoal parent = unwrapComposition(goals[parentDepth].back());

		if(parent->hasHash())
		{
			auto solution = parentDepth < depth ? aggregateGoals(parentDepth + 1, goal) : goal;

#if NKAI_TRACE_LEVEL >= 2
			logAi->trace("Adding %s to decomosition cache of %s at level %d", solution->toString(), parent->toString(), parentDepth);
#endif

			decompositionCache[parentDepth][parent].push_back(solution);

			if(trusted && parentDepth != 0)
			{
				decompositionCache[0][parent].push_back(solution);
				trusted = false;
			}
		}
	}
}

}
