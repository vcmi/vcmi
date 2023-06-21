/*
 * CPathfinder.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PathfinderOptions.h"

#include "../CConfigHandler.h"
#include "NodeStorage.h"
#include "PathfindingRules.h"
#include "CPathfinder.h"

PathfinderOptions::PathfinderOptions()
{
	useFlying = settings["pathfinder"]["layers"]["flying"].Bool();
	useWaterWalking = settings["pathfinder"]["layers"]["waterWalking"].Bool();
	useEmbarkAndDisembark = settings["pathfinder"]["layers"]["sailing"].Bool();
	useTeleportTwoWay = settings["pathfinder"]["teleports"]["twoWay"].Bool();
	useTeleportOneWay = settings["pathfinder"]["teleports"]["oneWay"].Bool();
	useTeleportOneWayRandom = settings["pathfinder"]["teleports"]["oneWayRandom"].Bool();
	useTeleportWhirlpool = settings["pathfinder"]["teleports"]["whirlpool"].Bool();

	useCastleGate = settings["pathfinder"]["teleports"]["castleGate"].Bool();

	lightweightFlyingMode = settings["pathfinder"]["lightweightFlyingMode"].Bool();
	oneTurnSpecialLayersLimit = settings["pathfinder"]["oneTurnSpecialLayersLimit"].Bool();
	originalMovementRules = settings["pathfinder"]["originalMovementRules"].Bool();
}

PathfinderConfig::PathfinderConfig(std::shared_ptr<INodeStorage> nodeStorage, std::vector<std::shared_ptr<IPathfindingRule>> rules):
	nodeStorage(std::move(nodeStorage)),
	rules(std::move(rules))
{
}

std::vector<std::shared_ptr<IPathfindingRule>> SingleHeroPathfinderConfig::buildRuleSet()
{
	return std::vector<std::shared_ptr<IPathfindingRule>>{
		std::make_shared<LayerTransitionRule>(),
			std::make_shared<DestinationActionRule>(),
			std::make_shared<MovementToDestinationRule>(),
			std::make_shared<MovementCostRule>(),
			std::make_shared<MovementAfterDestinationRule>()
	};
}

SingleHeroPathfinderConfig::~SingleHeroPathfinderConfig() = default;

SingleHeroPathfinderConfig::SingleHeroPathfinderConfig(CPathsInfo & out, CGameState * gs, const CGHeroInstance * hero)
	: PathfinderConfig(std::make_shared<NodeStorage>(out, hero), buildRuleSet())
{
	pathfinderHelper = std::make_unique<CPathfinderHelper>(gs, hero, options);
}

CPathfinderHelper * SingleHeroPathfinderConfig::getOrCreatePathfinderHelper(const PathNodeInfo & source, CGameState * gs)
{
	return pathfinderHelper.get();
}
