/*
* AIMovementToDestinationRule.h, part of VCMI engine
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
#include "../../../../lib/mapping/CMap.h"
#include "../../../../lib/mapObjects/MapObjects.h"

namespace AIPathfinding
{
	class AIMovementToDestinationRule : public MovementToDestinationRule
	{
	private:
		std::shared_ptr<AINodeStorage> nodeStorage;

	public:
		AIMovementToDestinationRule(std::shared_ptr<AINodeStorage> nodeStorage);

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override;
	};
}
