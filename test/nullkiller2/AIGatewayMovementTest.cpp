/*
 * AIGatewayMovementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "quest/QuestTest.h"

#include "lib/CPlayerState.h"
#include "lib/callback/CCallback.h"
#include "lib/callback/IClient.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER(0);

enum class MovementReply
{
	ExhaustAfterFirstStep,
	FailFirstStep
};

class RecordingMovementClient : public IClient
{
public:
	std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor, const BattleID &, const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		if(!movement)
			return ++lastRequestID;

		auto * hero = gameState->getHero(movement->hid);
		EXPECT_NE(hero, nullptr);
		EXPECT_FALSE(movement->path.empty());
		if(!hero || movement->path.empty())
			return ++lastRequestID;

		movementRequests++;
		requestedDestinations.push_back(movement->path.back());

		if(movementRequests > 1)
		{
			ADD_FAILURE() << "AI sent MoveHero after movement had already stopped; stale destination was " << movement->path.back().toString();
			applyMovement(*hero, movement->path.back(), TryMoveHero::FAILED, hero->movementPointsRemaining());
			return ++lastRequestID;
		}

		if(reply == MovementReply::FailFirstStep)
			applyMovement(*hero, movement->path.back(), TryMoveHero::FAILED, hero->movementPointsRemaining());
		else
			applyMovement(*hero, movement->path.back(), TryMoveHero::SUCCESS, 0);

		return ++lastRequestID;
	}

	void connect(CGameState & state, NK2AI::AIGateway & aiGateway, MovementReply replyMode)
	{
		gameState = &state;
		gateway = &aiGateway;
		reply = replyMode;
	}

	int movementRequests = 0;
	std::vector<int3> requestedDestinations;

private:
	void applyMovement(const CGHeroInstance & hero, const int3 & destination, TryMoveHero::EResult result, ui32 movePoints)
	{
		TryMoveHero movementResult;
		movementResult.id = hero.id;
		movementResult.start = hero.pos;
		movementResult.end = destination;
		movementResult.result = result;
		movementResult.movePoints = movePoints;

		gameState->apply(movementResult);
		gateway->heroMoved(movementResult, false);
	}

	CGameState * gameState = nullptr;
	NK2AI::AIGateway * gateway = nullptr;
	MovementReply reply = MovementReply::ExhaustAfterFirstStep;
	int lastRequestID = 0;
};

class AIGatewayMovementTest : public QuestTest
{
protected:
	void startGame(MovementReply reply)
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(36, false)
			.playerActive(PLAYER)
			.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, 1}});

		startWithMap(std::move(builder));
		revealMap(PLAYER);

		hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
		hero->setMovementPoints(2000);

		callback = std::make_shared<CCallback>(gameState(), std::optional<PlayerColor>{PLAYER}, &client);
		gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
		client.connect(*gameState(), *gateway, reply);
	}

	void revealMap(PlayerColor player)
	{
		auto * team = gameState()->getPlayerTeam(player);
		ASSERT_NE(team, nullptr);

		for(int z = 0; z < map()->levels(); ++z)
			for(int x = 0; x < map()->width; ++x)
				for(int y = 0; y < map()->height; ++y)
					team->fogOfWarMap[int3(x, y, z)] = 1;
	}

	int3 distantDestination() const
	{
		return hero->visitablePos() + int3(4, 0, 0);
	}

	RecordingMovementClient client;
	std::shared_ptr<CCallback> callback;
	std::unique_ptr<NK2AI::AIGateway> gateway;
	CGHeroInstance * hero = nullptr;
};
}

TEST_F(AIGatewayMovementTest, stopsRouteWhenMoveConsumesLastMovementPoint)
{
	startGame(MovementReply::ExhaustAfterFirstStep);

	const auto destination = distantDestination();

	EXPECT_FALSE(gateway->moveHeroToTile(destination, NK2AI::HeroPtr(hero, callback.get())));
	EXPECT_EQ(client.movementRequests, 1) << "the AI must not send a stale MoveHero after the hero reaches 0 movement points";
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_NE(hero->visitablePos(), destination);
}

TEST_F(AIGatewayMovementTest, throwsAndStopsRouteAfterFailedMovePacket)
{
	startGame(MovementReply::FailFirstStep);

	const auto startPosition = hero->visitablePos();

	EXPECT_THROW(gateway->moveHeroToTile(distantDestination(), NK2AI::HeroPtr(hero, callback.get())), NK2AI::cannotFulfillGoalException);
	EXPECT_EQ(client.movementRequests, 1) << "the AI must not continue through cached path nodes after TryMoveHero::FAILED";
	EXPECT_EQ(hero->visitablePos(), startPosition);
}
