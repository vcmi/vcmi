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

namespace NK2AI
{

using namespace Goals;

std::string ClusterBehavior::toString() const
{
	return "Unlock Clusters";
}

Goals::TGoalVec ClusterBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	auto clusters = aiNk->objectClusterizer->getLockedClusters();

	for(auto cluster : clusters)
	{
		vstd::concatenate(tasks, decomposeCluster(aiNk, cluster));
	}

	return tasks;
}

Goals::TGoalVec ClusterBehavior::decomposeCluster(const Nullkiller * aiNk, const std::shared_ptr<ObjectCluster> & cluster) const
{
	const auto * center = cluster->calculateCenter(aiNk->cc.get());
	auto paths = aiNk->pathfinder->getPathInfo(center->visitablePos(), aiNk->isObjectGraphAllowed());
	const auto blockerPos = cluster->blocker->visitablePos();
	std::vector<AIPath> blockerPaths;
	blockerPaths.reserve(paths.size());
	TGoalVec goals;

#if NK2AI_TRACE_LEVEL >= 2
	logAi->trace(
		"Checking cluster %s %s, found %d paths",
		cluster->blocker->getObjectName(),
		cluster->blocker->visitablePos().toString(),
		paths.size());
#endif

	for(auto path = paths.begin(); path != paths.end();)
	{
#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace("ClusterBehavior::decomposeCluster Checking path (of %d paths) %s", paths.size(), path->toString());
#endif

		const auto * blocker = aiNk->objectClusterizer->getBlocker(*path);
		if(blocker != cluster->blocker)
		{
			path = paths.erase(path);
			continue;
		}

		blockerPaths.push_back(*path);
		AIPath & clonedPath = blockerPaths.back();
		clonedPath.nodes.clear();

		// The pathfinding algorithm naturally returns paths in reverse order (destination → source)
		// We need to find the blocker position and include only nodes up to that point
		for(auto node = path->nodes.rbegin(); node != path->nodes.rend(); ++node)
		{
			clonedPath.nodes.insert(clonedPath.nodes.begin(), *node);
			if(node->coord == blockerPos || aiNk->cc->getGuardingCreaturePosition(node->coord) == blockerPos)
				break;
		}

		for(auto & node : clonedPath.nodes)
			node.parentIndex -= path->nodes.size() - clonedPath.nodes.size();

		if(clonedPath.nodes.empty())
			throw std::runtime_error("ClusterBehavior::decomposeCluster clonedPath has no nodes");
		if(clonedPath.nodes.size() < 2 && clonedPath.nodes.front().targetHero != clonedPath.targetHero)
		{
			logAi->warn("ClusterBehavior::decomposeCluster clonedPath not satisfying AIPath::targetNode() requirements. Original path: %s", path->toString());
			clonedPath.targetHero = clonedPath.nodes.front().targetHero;
			// Has to log after the fix is applied in the line above, otherwise toString will crash
			logAi->warn("ClusterBehavior::decomposeCluster clonedPath after fix: %s", clonedPath.toString());
		}

#if NK2AI_TRACE_LEVEL >= 2
		logAi->trace("Unlock path found %s", blockerPaths.back().toString());
#endif

		++path;
	}

#if NK2AI_TRACE_LEVEL >= 2
	logAi->trace("Decompose unlock paths");
#endif

	auto unlockTasks = CaptureObjectsBehavior::getVisitGoals(blockerPaths, aiNk, cluster->blocker);

	for(int i = 0; i < paths.size(); i++)
	{
		if(i >= unlockTasks.size())
			throw std::runtime_error("ClusterBehavior::decomposeCluster unlockTasks size mismatch with paths size");

		if(unlockTasks[i]->invalid())
			continue;

		auto path = paths[i];
		auto elementarUnlock = sptr(UnlockCluster(cluster, path));
		goals.push_back(sptr(Composition().addNext(elementarUnlock).addNext(unlockTasks[i])));
	}

	return goals;
}

}
