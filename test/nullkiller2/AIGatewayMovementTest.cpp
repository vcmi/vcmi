/*
 * AIGatewayMovementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"
#include "nullkiller2/ScriptedMovementClient.h"

#include "lib/callback/CCallback.h"

namespace
{
const PlayerColor PLAYER(0);

class Nullkiller2_AIGateway : public NullkillerTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PLAYER)
			.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, 1}})
			.terrain(UNREACHABLE_TILE, TerrainId::ROCK);

		startWithMap(std::move(builder));
		revealMap(PLAYER);

		hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
		hero->setMovementPoints(2000);

		client = std::make_unique<ScriptedMovementClient>(gameState());
		callback = makeCallback(PLAYER, client.get());
		gateway = makeGateway(callback);
		client->notify(*gateway);
	}

	NK2AI::HeroMovementResult moveHeroTo(const int3 & destination)
	{
		return gateway->moveHeroToTile(destination, NK2AI::HeroPtr(hero, callback.get()));
	}

	int3 distantDestination() const
	{
		return hero->visitablePos() + int3(4, 0, 0);
	}

	const int3 UNREACHABLE_TILE{20, 20, 0};

	std::unique_ptr<ScriptedMovementClient> client;
	std::shared_ptr<CCallback> callback;
	std::unique_ptr<NK2AI::AIGateway> gateway;
	CGHeroInstance * hero = nullptr;
};
}

TEST_F(Nullkiller2_AIGateway, stopsRouteWhenMoveConsumesLastMovementPoint)
{
	startGame();
	client->exhaustMovement();

	const auto destination = distantDestination();

	EXPECT_EQ(moveHeroTo(destination), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "a hero without movement points must not request further steps";
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_NE(hero->visitablePos(), destination);
}

TEST_F(Nullkiller2_AIGateway, throwsAndStopsRouteAfterFailedMovePacket)
{
	startGame();
	client->failMovement();

	const auto startPosition = hero->visitablePos();

	EXPECT_THROW(moveHeroTo(distantDestination()), NK2AI::cannotFulfillGoalException);
	EXPECT_EQ(client->movementRequests, 1) << "a rejected step must end the route";
	EXPECT_EQ(hero->visitablePos(), startPosition);
}

TEST_F(Nullkiller2_AIGateway, reportsUnreachableTileAsBlocked)
{
	startGame();

	EXPECT_EQ(moveHeroTo(UNREACHABLE_TILE), NK2AI::HeroMovementResult::BLOCKED);
	EXPECT_EQ(client->movementRequests, 0);
}
