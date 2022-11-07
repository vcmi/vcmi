/*
* AIMovementToDestinationRule.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIMovementToDestinationRule.h"

namespace NKAI
{
namespace AIPathfinding
{
	AIMovementToDestinationRule::AIMovementToDestinationRule(std::shared_ptr<AINodeStorage> nodeStorage)
		: nodeStorage(nodeStorage)
	{
	}

	void AIMovementToDestinationRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		auto blocker = getBlockingReason(source, destination, pathfinderConfig, pathfinderHelper);

		if(blocker == BlockingReason::NONE)
			return;

		if(blocker == BlockingReason::DESTINATION_BLOCKED
			&& destination.action == CGPathNode::EMBARK
			&& nodeStorage->getAINode(destination.node)->specialAction)
		{
			return;
		}

		if(blocker == BlockingReason::SOURCE_GUARDED && nodeStorage->getAINode(source.node)->actor->allowBattle)
		{
#if NKAI_PATHFINDER_TRACE_LEVEL >= 1
			logAi->trace(
				"Bypass src guard while moving from %s to %s",
				source.coord.toString(),
				destination.coord.toString());
#endif
			return;
		}

		destination.blocked = true;
	}
}

}
