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
	client->drainMovement(0);

	const auto destination = distantDestination();

	EXPECT_EQ(moveHeroTo(destination), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "a hero without movement points must not request further steps";
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_NE(hero->visitablePos(), destination);
}

TEST_F(Nullkiller2_AIGateway, stopsRouteWhenMoveLeavesFewerMovementPointsThanPlanned)
{
	startGame();
	client->drainMovement(1);

	EXPECT_EQ(moveHeroTo(distantDestination()), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "the server would reject a step the hero can no longer afford";
	EXPECT_EQ(hero->movementPointsRemaining(), 1);
}

TEST_F(Nullkiller2_AIGateway, stopsRouteWhenStepBecomesMoreExpensiveThanPlanned)
{
	const HeroTypeID fortressHero(112);
	const CreatureID gnoll(98);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PLAYER)
		.hero({5, 5, 0}, fortressHero, PLAYER)
		.heroGarrison({{gnoll, 1}});
	for(int x = 4; x < 10; ++x)
		builder.terrain({x, 5, 0}, TerrainId::SWAMP);

	startWithMap(std::move(builder));
	revealMap(PLAYER);
	hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);

	// native swamp costs 100 per step, so the first step leaves the planned 160 points
	hero->setMovementPoints(260);
	client = std::make_unique<ScriptedMovementClient>(gameState());
	client->drainMovement(160);
	// non-native unit given by a map event makes swamp cost 175
	client->afterMovement = [this]() { hero->addToSlot(SlotID(1), CreatureID::ARCHER, 1); };
	callback = makeCallback(PLAYER, client.get());
	gateway = makeGateway(callback);
	client->notify(*gateway);

	EXPECT_EQ(moveHeroTo(distantDestination()), NK2AI::HeroMovementResult::PROGRESSED);
	EXPECT_EQ(client->movementRequests, 1) << "the server would reject a step that costs more than the hero has";
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
