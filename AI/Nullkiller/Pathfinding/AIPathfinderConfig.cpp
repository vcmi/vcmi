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
#include "../Engine/Nullkiller.h"

#include "../../../lib/pathfinder/CPathfinder.h"

namespace NKAI
{
namespace AIPathfinding
{
	std::vector<std::shared_ptr<IPathfindingRule>> makeRuleset(
		CPlayerSpecificInfoCallback * cb,
		Nullkiller * ai,
		std::shared_ptr<AINodeStorage> nodeStorage,
		bool allowBypassObjects)
	{
			std::vector<std::shared_ptr<IPathfindingRule>> rules = {
				std::make_shared<AILayerTransitionRule>(cb, ai, nodeStorage),
				std::make_shared<DestinationActionRule>(),
				std::make_shared<AIMovementToDestinationRule>(nodeStorage, allowBypassObjects),
				std::make_shared<MovementCostRule>(),
				std::make_shared<AIPreviousNodeRule>(nodeStorage),
				std::make_shared<AIMovementAfterDestinationRule>(ai, cb, nodeStorage, allowBypassObjects)
			};

		return rules;
	}

	AIPathfinderConfig::AIPathfinderConfig(
		CPlayerSpecificInfoCallback * cb,
		Nullkiller * ai,
		std::shared_ptr<AINodeStorage> nodeStorage,
		bool allowBypassObjects)
		:PathfinderConfig(nodeStorage, *cb, makeRuleset(cb, ai, nodeStorage, allowBypassObjects)), aiNodeStorage(nodeStorage)
	{
		options.canUseCast = true;
		options.allowLayerTransitioningAfterBattle = true;
		options.useTeleportWhirlpool = true;
		options.forceUseTeleportWhirlpool = true;
		options.useTeleportOneWay = ai->settings->isOneWayMonolithUsageAllowed();
		options.useTeleportOneWayRandom = ai->settings->isOneWayMonolithUsageAllowed();
	}

	AIPathfinderConfig::~AIPathfinderConfig() = default;

	CPathfinderHelper * AIPathfinderConfig::getOrCreatePathfinderHelper(const PathNodeInfo & source, const IGameInfoCallback & gameInfo)
	{
		auto hero = aiNodeStorage->getHero(source.node);
		auto & helper = pathfindingHelpers[hero];

		if(!helper)
		{
			helper.reset(new CPathfinderHelper(gameInfo, hero, options));
		}

		return helper.get();
	}
}

}
