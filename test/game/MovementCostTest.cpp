/*
 * MovementCostTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"
#include "../../server/CGameHandler.h"
#include "../../server/IGameServer.h"

namespace
{

constexpr ui8 FAVORABLE_WINDS_FLAG = 128;

class GameServerMock : public IGameServer
{
public:
	MOCK_METHOD(void, setState, (EServerState value), (override));
	MOCK_METHOD(EServerState, getState, (), (const, override));
	MOCK_METHOD(bool, isPlayerHost, (const PlayerColor & color), (const, override));
	MOCK_METHOD(bool, hasPlayerAt, (PlayerColor player, GameConnectionID connectionID), (const, override));
	MOCK_METHOD(bool, hasBothPlayersAtSameConnection, (PlayerColor left, PlayerColor right), (const, override));
	MOCK_METHOD(void, applyPack, (CPackForClient & pack), (override));
	MOCK_METHOD(void, sendPack, (CPackForClient & pack, GameConnectionID connectionID), (override));
};

}

class MovementCostTest : public GameStateTest
{
};

TEST_F(MovementCostTest, usesExplicitDestinationLayer)
{
	startTestGame();

	const auto heroes = gameState->getPlayerState(PlayerColor(0))->getHeroes();
	ASSERT_FALSE(heroes.empty());
	const auto * hero = heroes.front();

	const int3 sourcePosition = hero->visitablePos();
	const int3 destinationPosition = sourcePosition + int3(1, 0, 0);
	TerrainTile sourceTile = *gameState->getTile(sourcePosition);
	TerrainTile destinationTile = *gameState->getTile(destinationPosition);
	sourceTile.extTileFlags |= FAVORABLE_WINDS_FLAG;

	CPathfinderHelper helper(*gameState, hero, PathfinderOptions(*gameState));
	const int landCost = helper.getMovementCost(
		sourcePosition, destinationPosition, EPathfindingLayer::LAND, 1000, false, &sourceTile, &destinationTile);
	const int sailCost = helper.getMovementCost(
		sourcePosition, destinationPosition, EPathfindingLayer::SAIL, 1000, false, &sourceTile, &destinationTile);

	EXPECT_EQ(sailCost, static_cast<int>(landCost * 2.0 / 3));
}

TEST_F(MovementCostTest, pathfinderAndExecutorUseSameSailCost)
{
	startTestGame();

	const auto heroes = gameState->getPlayerState(PlayerColor(0))->getHeroes();
	ASSERT_FALSE(heroes.empty());
	const auto * hero = heroes.front();
	const int3 sourcePosition = hero->visitablePos();
	const int3 embarkPosition = sourcePosition + int3(1, 0, 0);
	const int3 destinationPosition = sourcePosition + int3(2, 0, 0);

	auto & embarkTile = gameState->getMap().getTile(embarkPosition);
	auto & destinationTile = gameState->getMap().getTile(destinationPosition);
	embarkTile.terrainType = ETerrainId::WATER;
	destinationTile.terrainType = ETerrainId::WATER;

	testing::NiceMock<GameServerMock> server;
	ON_CALL(server, applyPack(testing::_)).WillByDefault([this](CPackForClient & pack)
	{
		gameState->apply(pack);
	});
	CGameHandler gameHandler(server, gameState);
	const int initialMovementPoints = hero->movementPointsRemaining();
	gameHandler.createBoat(embarkPosition, BoatId::CASTLE, hero->getOwner());
	ASSERT_FALSE(gameHandler.moveHero(
		hero->id,
		hero->convertFromVisitablePos(embarkPosition),
		EMovementMode::STANDARD,
		false,
		hero->getOwner(),
		EPathfindingLayer::AUTO));
	EXPECT_EQ(hero->visitablePos(), sourcePosition);
	EXPECT_EQ(hero->movementPointsRemaining(), initialMovementPoints);

	ASSERT_TRUE(gameHandler.moveHero(
		hero->id,
		hero->convertFromVisitablePos(embarkPosition),
		EMovementMode::STANDARD,
		false,
		hero->getOwner(),
		EPathfindingLayer::SAIL));
	ASSERT_TRUE(hero->inBoat());
	gameHandler.setMovePoints(hero->id, initialMovementPoints);
	gameState->getMap().getTile(embarkPosition).extTileFlags |= FAVORABLE_WINDS_FLAG;

	CPathsInfo paths(gameState->getMapSize(), hero);
	auto config = std::make_shared<SingleHeroPathfinderConfig>(paths, *gameState, hero);
	CPathfinder pathfinder(*gameState, config);
	pathfinder.calculatePaths();

	const auto * destinationNode = paths.getNode(destinationPosition, EPathfindingLayer::SAIL);
	ASSERT_TRUE(destinationNode->reachable());
	ASSERT_EQ(destinationNode->turns, 0);
	ASSERT_EQ(destinationNode->action, EPathNodeAction::NORMAL);
	const int expectedMovementPoints = destinationNode->moveRemains;
	ASSERT_GT(expectedMovementPoints, 0);

	ASSERT_TRUE(gameHandler.moveHero(
		hero->id,
		hero->convertFromVisitablePos(destinationPosition),
		EMovementMode::STANDARD,
		false,
		hero->getOwner(),
		destinationNode->layer));
	EXPECT_EQ(hero->movementPointsRemaining(), expectedMovementPoints);
}
