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
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../AIUtility.h"
#include "../Markers/UnlockCluster.h"
#include "../Goals/Composition.h"
#include "../Behaviors/CaptureObjectsBehavior.h"

namespace NKAI
{

using namespace Goals;

std::string ClusterBehavior::toString() const
{
	return "Unlock Clusters";
}

Goals::TGoalVec ClusterBehavior::decompose(const Nullkiller * ai) const
{
	Goals::TGoalVec tasks;
	auto clusters = ai->objectClusterizer->getLockedClusters();

	for(auto cluster : clusters)
	{
		vstd::concatenate(tasks, decomposeCluster(ai, cluster));
	}

	return tasks;
}

Goals::TGoalVec ClusterBehavior::decomposeCluster(const Nullkiller * ai, std::shared_ptr<ObjectCluster> cluster) const
{
	auto center = cluster->calculateCenter(ai->cb.get());
	auto paths = ai->pathfinder->getPathInfo(center->visitablePos(), ai->isObjectGraphAllowed());

	auto blockerPos = cluster->blocker->visitablePos();
	std::vector<AIPath> blockerPaths;

	blockerPaths.reserve(paths.size());

	TGoalVec goals;

#if NKAI_TRACE_LEVEL >= 2
	logAi->trace(
		"Checking cluster %s %s, found %d paths",
		cluster->blocker->getObjectName(),
		cluster->blocker->visitablePos().toString(),
		paths.size());
#endif

	for(auto path = paths.begin(); path != paths.end();)
	{
#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("ClusterBehavior Checking path %s", path->toString());
#endif

		auto blocker = ai->objectClusterizer->getBlocker(*path);

		if(blocker != cluster->blocker)
		{
			path = paths.erase(path);
			continue;
		}

		blockerPaths.push_back(*path);

		AIPath & clonedPath = blockerPaths.back();

		clonedPath.nodes.clear();

		for(auto node = path->nodes.rbegin(); node != path->nodes.rend(); node++)
		{
			clonedPath.nodes.insert(clonedPath.nodes.begin(), *node);

			if(node->coord == blockerPos || ai->cb->getGuardingCreaturePosition(node->coord) == blockerPos)
				break;
		}

		for(auto & node : clonedPath.nodes)
			node.parentIndex -= path->nodes.size() - clonedPath.nodes.size();

#if NKAI_TRACE_LEVEL >= 2
		logAi->trace("Unlock path found %s", blockerPaths.back().toString());
#endif
		path++;
	}

#if NKAI_TRACE_LEVEL >= 2
	logAi->trace("Decompose unlock paths");
#endif

	auto unlockTasks = CaptureObjectsBehavior::getVisitGoals(blockerPaths, ai, cluster->blocker);

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

}
