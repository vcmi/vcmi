/*
* AIPathfinderConfig.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AIPathfinderConfig.h"
#include "Rules/AILayerTransitionRule.h"
#include "Rules/AIMovementAfterDestinationRule.h"
#include "Rules/AIMovementToDestinationRule.h"
#include "Rules/AIPreviousNodeRule.h"
#include "../Engine//Nullkiller.h"

#include "../../../lib/pathfinder/CPathfinder.h"

namespace NKAI
{
namespace AIPathfinding
{
	std::vector<std::shared_ptr<IPathfindingRule>> makeRuleset(
		CPlayerSpecificInfoCallback * cb,
		Nullkiller * ai,
		std::shared_ptr<AINodeStorage> nodeStorage)
	{
		std::vector<std::shared_ptr<IPathfindingRule>> rules = {
			std::make_shared<AILayerTransitionRule>(cb, ai, nodeStorage),
			std::make_shared<DestinationActionRule>(),
			std::make_shared<AIMovementToDestinationRule>(nodeStorage),
			std::make_shared<MovementCostRule>(),
			std::make_shared<AIPreviousNodeRule>(nodeStorage),
			std::make_shared<AIMovementAfterDestinationRule>(cb, nodeStorage)
		};

		return rules;
	}

	AIPathfinderConfig::AIPathfinderConfig(
		CPlayerSpecificInfoCallback * cb,
		Nullkiller * ai,
		std::shared_ptr<AINodeStorage> nodeStorage)
		:PathfinderConfig(nodeStorage, makeRuleset(cb, ai, nodeStorage)), aiNodeStorage(nodeStorage)
	{
	}

	AIPathfinderConfig::~AIPathfinderConfig() = default;

	CPathfinderHelper * AIPathfinderConfig::getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs)
	{
		auto hero = aiNodeStorage->getHero(source.node);
		auto & helper = pathfindingHelpers[hero];

		if(!helper)
		{
			helper.reset(new CPathfinderHelper(gs, hero, options));
		}

		return helper.get();
	}
}

}
