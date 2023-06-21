/*
* AIMovementAfterDestinationRule.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../AINodeStorage.h"
#include "../../AIGateway.h"
#include "../../../../CCallback.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "../../../../lib/pathfinder/PathfindingRules.h"

namespace NKAI
{
namespace AIPathfinding
{
	class AIMovementAfterDestinationRule : public MovementAfterDestinationRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		std::shared_ptr<AINodeStorage> nodeStorage;

	public:
		AIMovementAfterDestinationRule(CPlayerSpecificInfoCallback * cb, std::shared_ptr<AINodeStorage> nodeStorage);

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override;

	private:
		bool bypassDestinationGuards(
			std::vector<const CGObjectInstance *> destGuardians,
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const;

		bool bypassRemovableObject(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const;

		bool bypassBlocker(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const;

		bool bypassBattle(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const;

		bool bypassQuest(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const;
	};
}

}
