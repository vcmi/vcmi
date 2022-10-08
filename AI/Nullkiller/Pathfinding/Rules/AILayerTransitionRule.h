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
#include "../../../../CCallback.h"
#include "../../../../lib/mapping/CMap.h"
#include "../../../../lib/mapObjects/MapObjects.h"

namespace NKAI
{
namespace AIPathfinding
{
	class AILayerTransitionRule : public LayerTransitionRule
	{
	private:
		CPlayerSpecificInfoCallback * cb;
		Nullkiller * ai;
		std::map<int3, std::shared_ptr<const BuildBoatAction>> virtualBoats;
		std::shared_ptr<AINodeStorage> nodeStorage;
		std::map<const CGHeroInstance *, std::shared_ptr<const SummonBoatAction>> summonableVirtualBoats;

	public:
		AILayerTransitionRule(CPlayerSpecificInfoCallback * cb, Nullkiller * ai, std::shared_ptr<AINodeStorage> nodeStorage);

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

}
