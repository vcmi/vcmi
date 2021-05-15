/*
* AIPreviousNodeRule.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIPreviousNodeRule.h"

namespace AIPathfinding
{
	AIPreviousNodeRule::AIPreviousNodeRule(std::shared_ptr<AINodeStorage> nodeStorage)
		: nodeStorage(nodeStorage)
	{
	}

	void AIPreviousNodeRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		if(source.node->action == CGPathNode::ENodeAction::BLOCKING_VISIT || source.node->action == CGPathNode::ENodeAction::VISIT)
		{
			// we can not directly bypass objects, we need to interact with them first
			destination.node->theNodeBefore = source.node;
#ifdef VCMI_TRACE_PATHFINDER
			logAi->trace(
				"Link src node %s to destination node %s while bypassing visitable obj",
				source.coord.toString(),
				destination.coord.toString());
#endif
			return;
		}
	}
}
