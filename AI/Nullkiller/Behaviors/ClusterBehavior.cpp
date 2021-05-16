/*
* RecruitHeroBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ClusterBehavior.h"
#include "../VCAI.h"
#include "../Engine/Nullkiller.h"
#include "../AIhelper.h"
#include "../AIUtility.h"
#include "../Goals/UnlockCluster.h"
#include "../Goals/Composition.h"
#include "../Behaviors/CaptureObjectsBehavior.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

std::string ClusterBehavior::toString() const
{
	return "Unlock Clusters";
}

Goals::TGoalVec ClusterBehavior::decompose() const
{
	Goals::TGoalVec tasks;
	auto clusters = ai->nullkiller->objectClusterizer->getLockedClusters();

	for(auto cluster : clusters)
	{
		vstd::concatenate(tasks, decomposeCluster(cluster));
	}

	return tasks;
}

Goals::TGoalVec ClusterBehavior::decomposeCluster(std::shared_ptr<ObjectCluster> cluster) const
{
	auto center = cluster->calculateCenter();
	auto paths = ai->ah->getPathsToTile(center->visitablePos());
	auto blockerPos = cluster->blocker->visitablePos();
	std::vector<AIPath> blockerPaths;

	blockerPaths.reserve(paths.size());

	TGoalVec goals;

#if AI_TRACE_LEVEL >= 2
	logAi->trace(
		"Checking cluster %s, found %d paths",
		cluster->blocker->getObjectName(),
		cluster->blocker->visitablePos().toString(),
		paths.size());
#endif

	for(auto path = paths.begin(); path != paths.end();)
	{
#if AI_TRACE_LEVEL >= 2
		logAi->trace("Checking path %s", path->toString());
#endif

		auto blocker = ai->nullkiller->objectClusterizer->getBlocker(*path);

		if(blocker != cluster->blocker)
		{
			path = paths.erase(path);
			continue;
		}

		blockerPaths.push_back(*path);

		blockerPaths.back().nodes.clear();

		for(auto node = path->nodes.rbegin(); node != path->nodes.rend(); node++)
		{
			blockerPaths.back().nodes.insert(blockerPaths.back().nodes.begin(), *node);

			if(node->coord == blockerPos)
				break;
		}

#if AI_TRACE_LEVEL >= 2
		logAi->trace("Unlock path found %s", blockerPaths.back().toString());
#endif
		path++;
	}

#if AI_TRACE_LEVEL >= 2
	logAi->trace("Decompose unlock paths");
#endif

	auto unlockTasks = CaptureObjectsBehavior::getVisitGoals(blockerPaths);

	for(int i = 0; i < paths.size(); i++)
	{
		if(unlockTasks[i]->invalid())
			continue;

		auto path = paths[i];
		auto elementarUnlock = sptr(UnlockCluster(cluster, path));

		goals.push_back(sptr(Composition().addNext(elementarUnlock).addNext(unlockTasks[i])));
	}

	return goals;
}
