/*
 * WaterExplorationBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "AI/Nullkiller2/Behaviors/ExplorationBehavior.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"
#include "AI/Nullkiller2/Pathfinding/Actions/BoatActions.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/mapping/TerrainTile.h"

namespace
{
const PlayerColor PLAYER(0);

/// Hero on the southern shore next to a shipyard, open sea to the north
TinyH3M::TinyH3MBuilder makeShipyardMap(PlayerColor shipyardOwner)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2WaterExploration")
		.playerActive(PLAYER)
		.hero({10, 22, 0}, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 100}})
		.shipyard({10, 18, 0}, shipyardOwner);

	for(int y = 0; y <= 17; ++y)
	{
		for(int x = 0; x < 36; ++x)
			builder.terrain({x, y, 0}, TerrainId::WATER);
	}

	return builder;
}

class Nullkiller2_Behaviors_WaterExploration : public NullkillerTest
{
public:
	/// Starts the game with everything visible except a patch of sea beyond walking sight
	void start(PlayerColor shipyardOwner)
	{
		startWithMap(makeShipyardMap(shipyardOwner));
		setMapVisibility(PLAYER, true);

		shipyard = findAll<CGShipyard>().front();
		gameState()->getPlayerTeam(PLAYER)->fogOfWarMap[shipyard->bestLocation() + int3(0, -10, 0)] = 0;
	}

	void setResource(GameResID resource, int amount)
	{
		gameState()->players.at(PLAYER).resources[resource] = amount;
	}

	void preparePlanning()
	{
		gateway = makeGateway(PLAYER);
		findAll<CGHeroInstance>().front()->setMovementPoints(2500);
		NullkillerTest::preparePlanning(*gateway);
	}

	NK2AI::Nullkiller & ai()
	{
		return *gateway->nullkiller;
	}

	NK2AI::Goals::TGoalVec explorationGoals()
	{
		return NK2AI::Goals::ExplorationBehavior().decompose(&ai());
	}

	/// Returns the last hero route of the goal, which is the one it executes
	const NK2AI::AIPath * routeOf(const NK2AI::Goals::TSubgoal & goal)
	{
		const NK2AI::AIPath * result = nullptr;
		for(const auto & part : goal->decompose(&ai()))
		{
			if(const auto * execute = dynamic_cast<const NK2AI::Goals::ExecuteHeroChain *>(part.get()))
				result = &execute->getPath();
		}
		return result;
	}

	NK2AI::Goals::TSubgoal findResourceReservation()
	{
		for(const auto & goal : explorationGoals())
		{
			const auto parts = goal->decompose(&ai());
			if(std::ranges::any_of(parts, [](const auto & part) { return part->goalType == NK2AI::Goals::SAVE_RESOURCES; }))
				return goal;
		}
		return {};
	}

	const CGShipyard * shipyard = nullptr;
	std::unique_ptr<NK2AI::AIGateway> gateway;
};
}

TEST_F(Nullkiller2_Behaviors_WaterExploration, ownShipyardRouteBuildsBoatToReachHiddenSea)
{
	start(PLAYER);
	preparePlanning();

	const auto goals = explorationGoals();
	const bool sailsFromShipyard = std::ranges::any_of(goals, [this](const auto & goal)
	{
		const auto * route = routeOf(goal);
		if(!route || route->getFirstBlockedAction() || !map()->getTile(route->targetTile()).isWater())
			return false;

		return std::ranges::any_of(route->nodes, [](const NK2AI::AIPathNodeInfo & node)
		{
			return std::dynamic_pointer_cast<const NK2AI::AIPathfinding::BuildBoatAction>(node.specialAction) != nullptr;
		});
	});

	EXPECT_TRUE(sailsFromShipyard);
}

TEST_F(Nullkiller2_Behaviors_WaterExploration, uncapturedShipyardIsVisitedFirst)
{
	start(PlayerColor::NEUTRAL);
	preparePlanning();

	NK2AI::Goals::TGoalVec tasks;
	ai().decomposer->decompose(tasks, NK2AI::Goals::sptr(NK2AI::Goals::ExplorationBehavior()), 10);
	const bool visitsShipyard = std::ranges::any_of(tasks, [this](const auto & task)
	{
		const auto * route = routeOf(task);
		return route && route->targetTile() == shipyard->visitablePos();
	});

	EXPECT_TRUE(visitsShipyard);
}

TEST_F(Nullkiller2_Behaviors_WaterExploration, missingBoatResourcesAreReservedOnce)
{
	start(PLAYER);
	setResource(EGameResID::WOOD, 7);
	setResource(EGameResID::GOLD, 10000);
	preparePlanning();

	const auto reservation = findResourceReservation();
	ASSERT_TRUE(reservation);
	reservation->asTask()->accept(gateway.get());

	EXPECT_EQ(ai().getLockedResources()[EGameResID::WOOD], 10);
	EXPECT_FALSE(findResourceReservation());
}
