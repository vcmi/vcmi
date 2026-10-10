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

#include "mock/GameHandlerTestClient.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER(0);
const PlayerColor ENEMY(1);
const int3 HERO_ANCHOR_POS(6, 5, 0);
const int3 HERO_POS(5, 5, 0);
const int3 SCROLL_POS(6, 5, 0);
const int3 BEYOND_SCROLL_POS(7, 5, 0);

class MoveCountingClient : public GameHandlerTestClient
{
public:
	using GameHandlerTestClient::GameHandlerTestClient;

	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override
	{
		if(dynamic_cast<const MoveHero *>(&request))
			++movementRequests;

		return GameHandlerTestClient::sendRequest(request, player, waitTillRealize);
	}

	int movementRequests = 0;
};

NK2AI::AIPathNodeInfo pathNode(const CGHeroInstance & hero, const int3 & coordinate)
{
	NK2AI::AIPathNodeInfo node{};
	node.coord = coordinate;
	node.layer = EPathfindingLayer::LAND;
	node.targetHero = &hero;
	node.parentIndex = -1;
	node.chainMask = 1;
	node.turns = 0;
	return node;
}

class ExecuteHeroChainMovementTest : public NullkillerTest
{
protected:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.hero(HERO_ANCHOR_POS, HeroTypeID(0), PLAYER)
			.heroGarrison({{CreatureID(0), 1}})
			.scroll(SCROLL_POS, SpellID(0))
			.playerActive(ENEMY)
			.hero({30, 30, 0}, HeroTypeID(1), ENEMY);

		startWithMap(std::move(builder));
	}
};
}

TEST_F(ExecuteHeroChainMovementTest, blockingVisitOnRouteStopsChainForReplanning)
{
	startGame();
	revealMap(PLAYER);

	auto * hero = findHeroByOwner(PLAYER);
	auto * scroll = findFirst<CGArtifact>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(scroll, nullptr);
	ASSERT_EQ(hero->visitablePos(), HERO_POS);
	ASSERT_EQ(scroll->visitablePos(), SCROLL_POS);
	const auto scrollID = scroll->id;
	hero->setMovementPoints(2000);

	MoveCountingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	NK2AI::AIPath path;
	path.targetHero = hero;
	path.heroArmy = hero;
	path.chainMask = 1;
	path.nodes.push_back(pathNode(*hero, BEYOND_SCROLL_POS));
	path.nodes.push_back(pathNode(*hero, SCROLL_POS));
	path.nodes.push_back(pathNode(*hero, HERO_POS));

	EXPECT_NO_THROW(NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()));
	EXPECT_EQ(gameState()->getObjInstance(scrollID), nullptr) << "the hero must pick up the scroll";
	EXPECT_EQ(hero->visitablePos(), HERO_POS) << "picking up a scroll does not move the hero";
	EXPECT_EQ(client.movementRequests, 1) << "the chain must stop after the pickup instead of following the stale route";
}

TEST_F(ExecuteHeroChainMovementTest, repeatedIdleChainFailsForReplanning)
{
	startGame();

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	hero->setMovementPoints(2000);

	MoveCountingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	NK2AI::AIPath path;
	path.targetHero = hero;
	path.heroArmy = hero;
	path.chainMask = 1;
	path.nodes.push_back(pathNode(*hero, HERO_POS));

	EXPECT_NO_THROW(NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()));
	EXPECT_THROW(NK2AI::Goals::ExecuteHeroChain(path).accept(gateway.get()), NK2AI::cannotFulfillGoalException)
		<< "an idle chain counted as success would be selected again on every pass";
	EXPECT_EQ(client.movementRequests, 0);
}
