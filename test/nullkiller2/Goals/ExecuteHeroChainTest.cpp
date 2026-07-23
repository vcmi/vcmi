/*
 * ExecuteHeroChainTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"

#include "mock/TinyH3MBuilder.h"
#include "quest/QuestTest.h"

#include "lib/callback/IClient.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER(0);
const ObjectInstanceID WHIRLPOOL(10);

class BlockingVisitClient : public IClient
{
public:
	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor,
		const BattleID &,
		const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor player, bool) override
	{
		const auto * movement = dynamic_cast<const MoveHero *>(&request);
		if(!movement)
			return ++lastRequestID;

		auto * hero = gameState->getHero(movement->hid);
		EXPECT_NE(hero, nullptr);
		EXPECT_FALSE(movement->path.empty());
		if(!hero || movement->path.empty())
			return ++lastRequestID;

		TryMoveHero result;
		result.id = hero->id;
		result.start = hero->pos;
		result.end = movement->path.back();
		result.movePoints = 0;
		result.result = TryMoveHero::BLOCKING_VISIT;
		gameState->apply(result);
		gateway->heroMoved(result, false);

		RemoveObject removeObject(blockingObject, player);
		gameState->apply(removeObject);
		++movementRequests;
		return ++lastRequestID;
	}

	void connect(CGameState & state, NK2AI::AIGateway & aiGateway, ObjectInstanceID object)
	{
		gameState = &state;
		gateway = &aiGateway;
		blockingObject = object;
	}

	int movementRequests = 0;

private:
	CGameState * gameState = nullptr;
	NK2AI::AIGateway * gateway = nullptr;
	ObjectInstanceID blockingObject;
	int lastRequestID = 0;
};

NK2AI::AIPathNodeInfo pathNode(
	const CGHeroInstance & hero,
	const int3 & coordinate,
	uint8_t turns)
{
	NK2AI::AIPathNodeInfo node{};
	node.coord = coordinate;
	node.layer = EPathfindingLayer::LAND;
	node.targetHero = &hero;
	node.parentIndex = -1;
	node.chainMask = 1;
	node.turns = turns;
	return node;
}

class ExecuteHeroChainMovementTest : public QuestTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.hero({6, 5, 0}, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID(0), 1}})
			.scroll({6, 5, 0}, SpellID(0));

		startWithMap(std::move(builder));
	}

	const CGObjectInstance * findScroll() const
	{
		for(const auto & object : map->objects)
		{
			if(object && object->ID == Obj::SPELL_SCROLL)
				return object.get();
		}

		return nullptr;
	}

	BlockingVisitClient client;
};
}

TEST(Nullkiller2_Goals_ExecuteHeroChain, recognizesCompletedAndStalledNodes)
{
	const int3 position(10, 10, 0);

	EXPECT_TRUE(NK2AI::Goals::shouldSkipCompletedChainNode(
		1, 2, position, position, int3(-1), ObjectInstanceID::NONE, ObjectInstanceID::NONE));
	EXPECT_FALSE(NK2AI::Goals::shouldSkipCompletedChainNode(
		0, 2, position, int3(11, 11, 0), position, WHIRLPOOL, WHIRLPOOL));
	EXPECT_TRUE(NK2AI::Goals::shouldSkipCompletedChainNode(
		0, 2, int3(12, 12, 0), int3(11, 11, 0), position, WHIRLPOOL, WHIRLPOOL));
}

TEST_F(ExecuteHeroChainMovementTest, blockingVisitIsProgressInsteadOfRouteFailure)
{
	startGame();

	auto * hero = findHeroByOwner(PLAYER);
	const auto * scroll = findScroll();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(scroll, nullptr);
	const auto startPosition = hero->visitablePos();
	const auto scrollPosition = scroll->visitablePos();
	const auto scrollID = scroll->id;
	hero->setMovementPoints(2000);

	const auto callback = makeCallback(PLAYER, &client);
	auto gateway = std::make_unique<NK2AI::AIGateway>();
	gateway->initGameInterface(std::shared_ptr<Environment>(), callback);
	client.connect(*gameState, *gateway, scrollID);

	NK2AI::AIPath path;
	path.targetHero = hero;
	path.heroArmy = hero;
	path.chainMask = 1;
	path.nodes.push_back(pathNode(*hero, scrollPosition + int3(2, 0, 0), 1));
	path.nodes.push_back(pathNode(*hero, scrollPosition + int3(1, 0, 0), 0));
	path.nodes.push_back(pathNode(*hero, scrollPosition, 0));

	EXPECT_NO_THROW(NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()));
	EXPECT_EQ(client.movementRequests, 1);
	EXPECT_EQ(gameState->getObjInstance(scrollID), nullptr);
	EXPECT_EQ(hero->visitablePos(), startPosition);
	EXPECT_EQ(hero->movementPointsRemaining(), 0);
}
