/*
 * MoveHeroBatchTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "../../server/CGameHandler.h"
#include "../../server/IGameServer.h"
#include "../../server/ServerNetPackVisitors.h"

#include "../quest/QuestTest.h"

#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"

namespace
{
const PlayerColor PLAYER(0);
constexpr GameConnectionID CONNECTION = GameConnectionID::FIRST_CONNECTION;

class RecordingGameServer : public IGameServer
{
public:
	explicit RecordingGameServer(CGameState * state)
		: gameState(state)
	{
	}

	void setState(EServerState value) override
	{
		state = value;
	}

	EServerState getState() const override
	{
		return state;
	}

	bool isPlayerHost(const PlayerColor & color) const override
	{
		return color == PLAYER;
	}

	bool hasPlayerAt(PlayerColor player, GameConnectionID connectionID) const override
	{
		return player == PLAYER && connectionID == CONNECTION;
	}

	bool hasBothPlayersAtSameConnection(PlayerColor left, PlayerColor right) const override
	{
		return left == PLAYER && right == PLAYER;
	}

	void applyPack(CPackForClient & pack) override
	{
		record(pack);
		gameState->apply(pack);
	}

	void sendPack(CPackForClient & pack, GameConnectionID connectionID) override
	{
		EXPECT_EQ(connectionID, CONNECTION);
		record(pack);
	}

	std::vector<std::string> systemMessages;
	std::vector<TryMoveHero> movementResults;

private:
	void record(CPackForClient & pack)
	{
		if(auto * message = dynamic_cast<SystemMessage *>(&pack))
			systemMessages.push_back(message->text.toString());

		if(auto * movement = dynamic_cast<TryMoveHero *>(&pack))
			movementResults.push_back(*movement);
	}

	CGameState * gameState;
	EServerState state = EServerState::GAMEPLAY;
};

class MoveHeroBatchTest : public QuestTest
{
protected:
	void SetUp() override
	{
		QuestTest::SetUp();

		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder.size(18, false)
			.playerActive(PLAYER)
			.hero({5, 5, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID::ARCHER, 1}});

		startWithMap(std::move(builder));
		gameState()->actingPlayers.insert(PLAYER);

		hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
	}

	int movementCostTo(const int3 & destination) const
	{
		CPathfinderHelper helper(*gameState(), hero, PathfinderOptions(*gameState()));
		return helper.getMovementCost(hero->visitablePos(), destination, nullptr, nullptr, hero->movementPointsRemaining());
	}

	CGHeroInstance * hero = nullptr;
};
}

TEST_F(MoveHeroBatchTest, stopsBatchWhenAppliedStepConsumesLastMovementPoint)
{
	const int3 firstTile = hero->visitablePos() + int3(1, 0, 0);
	const int3 secondTile = hero->visitablePos() + int3(2, 0, 0);
	const int movementCost = movementCostTo(firstTile);
	ASSERT_GT(movementCost, 0);

	hero->setMovementPoints(movementCost);

	RecordingGameServer server(gameState().get());
	CGameHandler handler(server, gameState());

	MoveHero pack(
		{
			hero->convertFromVisitablePos(firstTile),
			hero->convertFromVisitablePos(secondTile)
		},
		EPathfindingLayer::LAND,
		hero->id,
		false
	);
	pack.player = PLAYER;

	ApplyGhNetPackVisitor visitor(handler, CONNECTION);
	pack.visit(visitor);

	EXPECT_TRUE(visitor.getResult()) << "a stale tail after a legal batched step must not be reported as fishy";
	ASSERT_EQ(server.movementResults.size(), 1);
	EXPECT_EQ(server.movementResults.front().result, TryMoveHero::SUCCESS);
	EXPECT_EQ(hero->visitablePos(), firstTile);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
	EXPECT_TRUE(server.systemMessages.empty()) << "the original bug surfaced as a server-problem system message";
}
