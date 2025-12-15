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
#include "../../AIGateway.h"
#include "../Actions/BoatActions.h"
#include "../Actions/AdventureSpellCastMovementActions.h"
#include "../../../../lib/mapObjects/MapObjects.h"
#include "../../../../lib/pathfinder/PathfindingRules.h"

namespace NK2AI
{
namespace AIPathfinding
{
	class AILayerTransitionRule : public LayerTransitionRule
	{
		Nullkiller * aiNk;
		std::map<int3, std::shared_ptr<const BuildBoatAction>> virtualBoats;
		std::shared_ptr<AINodeStorage> nodeStorage;
		std::map<const CGHeroInstance *, std::shared_ptr<const SummonBoatAction>> summonableVirtualBoats;
		std::map<const CGHeroInstance *, std::shared_ptr<const WaterWalkingAction>> waterWalkingActions;
		std::map<const CGHeroInstance *, std::shared_ptr<const AirWalkingAction>> airWalkingActions;

	public:
		AILayerTransitionRule(Nullkiller * aiNk, std::shared_ptr<AINodeStorage> nodeStorage);

		virtual void process(
			const PathNodeInfo & source,
			CDestinationNodeInfo & destination,
			const PathfinderConfig * pathfinderConfig,
			CPathfinderHelper * pathfinderHelper
		) const override;

	private:
		void setup();
		void collectVirtualBoats();

		std::shared_ptr<const VirtualBoatAction> findVirtualBoat(
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source) const;

		bool tryUseSpecialAction(
			CDestinationNodeInfo & destination,
			const PathNodeInfo & source,
			std::shared_ptr<const SpecialAction> specialAction,
			EPathNodeAction targetAction) const;
	};
}

}
