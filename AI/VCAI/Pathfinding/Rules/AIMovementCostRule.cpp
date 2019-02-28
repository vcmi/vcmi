/*
* AIMovementCostRule.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIMovementCostRule.h"

namespace AIPathfinding
{
	AIMovementCostRule::AIMovementCostRule(std::shared_ptr<AINodeStorage> nodeStorage)
		: nodeStorage(nodeStorage)
	{
	}

	void AIMovementCostRule::process(
		const PathNodeInfo & source,
		CDestinationNodeInfo & destination,
		const PathfinderConfig * pathfinderConfig,
		CPathfinderHelper * pathfinderHelper) const
	{
		auto srcNode = nodeStorage->getAINode(source.node);
		auto dstNode = nodeStorage->getAINode(destination.node);
		auto srcActor = srcNode->actor;
		auto dstActor = dstNode->actor;

		if(srcActor == dstActor)
		{
			MovementCostRule::process(source, destination, pathfinderConfig, pathfinderHelper);

			return;
		}
		
		auto carrierActor = dstActor->carrierParent;
		auto otherActor = dstActor->otherParent;

		if(source.coord == destination.coord)
		{
			auto carrierNode = nodeStorage->getOrCreateNode(source.coord, source.node->layer, carrierActor).get();
			auto otherNode = nodeStorage->getOrCreateNode(source.coord, source.node->layer, otherActor).get();

			if(carrierNode->turns >= otherNode->turns)
			{
				destination.turn = carrierNode->turns;
				destination.cost = carrierNode->cost;

				return;
			}

			double waitingCost = otherNode->turns - carrierNode->turns - 1.0
				+ carrierNode->moveRemains / (double)pathfinderHelper->getMaxMovePoints(carrierNode->layer);

			destination.turn = otherNode->turns;
			destination.cost = waitingCost;
		}
		else
		{
			// TODO: exchange through sail->land border might be more sofisticated
			destination.blocked = true;
		}
	}
}
