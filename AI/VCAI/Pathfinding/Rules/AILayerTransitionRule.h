/*
* AILayerTransitionRule.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../AINodeStorage.h"
#include "../../VCAI.h"
#include "../Actions/BoatActions.h"
#include "../../../../CCallback.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "../../../../lib/pathfinder/PathfindingRules.h"

namespace AIPathfinding
{
	class AILayerTransitionRule : public LayerTransitionRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		VCAI * ai;
		std::map<int3, std::shared_ptr<const BuildBoatAction>> virtualBoats;
		std::shared_ptr<AINodeStorage> nodeStorage;
		std::shared_ptr<const SummonBoatAction> summonableVirtualBoat;

	public:
		AILayerTransitionRule(CPlayerSpecificInfoCallback * cb, VCAI * ai, std::shared_ptr<AINodeStorage> nodeStorage);

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper) const override;

	private:
		void setup();

		std::shared_ptr<const VirtualBoatAction> findVirtualBoat(
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source) const;

		bool tryEmbarkVirtualBoat(
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			std::shared_ptr<const VirtualBoatAction> virtualBoat) const;
	};
}
