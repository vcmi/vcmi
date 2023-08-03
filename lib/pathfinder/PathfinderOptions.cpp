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

#include "../GameSettings.h"
#include "../VCMI_Lib.h"
#include "NodeStorage.h"
#include "PathfindingRules.h"
#include "CPathfinder.h"

VCMI_LIB_NAMESPACE_BEGIN

PathfinderOptions::PathfinderOptions()
	: useFlying(true)
	, useWaterWalking(true)
	, useEmbarkAndDisembark(VLC->settings()->getBoolean(EGameSettings::PATHFINDER_USE_BOAT))
	, useTeleportTwoWay(VLC->settings()->getBoolean(EGameSettings::PATHFINDER_USE_MONOLITH_TWO_WAY))
	, useTeleportOneWay(VLC->settings()->getBoolean(EGameSettings::PATHFINDER_USE_MONOLITH_ONE_WAY_UNIQUE))
	, useTeleportOneWayRandom(VLC->settings()->getBoolean(EGameSettings::PATHFINDER_USE_MONOLITH_ONE_WAY_RANDOM))
	, useTeleportWhirlpool(VLC->settings()->getBoolean(EGameSettings::PATHFINDER_USE_WHIRLPOOL))
	, useCastleGate(false)
	, lightweightFlyingMode(false)
	, oneTurnSpecialLayersLimit(true)
	, originalMovementRules(false)
{
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

VCMI_LIB_NAMESPACE_END
