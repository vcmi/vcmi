/*
 * QuestActionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Goals/ExecuteHeroChain.h"
#include "AI/Nullkiller2/Pathfinding/AIPathfinder.h"
#include "AI/Nullkiller2/Pathfinding/Actions/QuestAction.h"
#include "AI/Nullkiller2/Pathfinding/GraphPaths.h"

#include "mock/GameHandlerTestServer.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "server/CGameHandler.h"

#include "lib/callback/CCallback.h"
#include "lib/callback/IClient.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"
#include "lib/serializer/CMemorySerializer.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const int3 HERO_ANCHOR_POS(5, 5, 0);
const int3 HERO_POS(4, 5, 0);
const int3 GATE_ANCHOR_POS(5, 6, 0);
const int3 GATE_POS(4, 6, 0);
const int3 TARGET_POS(4, 8, 0);

class GameHandlerClient : public IClient
{
public:
	explicit GameHandlerClient(CGameHandler & gameHandler)
		: gameHandler(gameHandler)
	{}

	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor,
		const BattleID &,
		const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor player, bool) override
	{
		request.player = player;
		request.requestID = ++lastRequestID;
		auto serverRequest = CMemorySerializer::deepCopy(request);
		gameHandler.handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *serverRequest);
		return lastRequestID;
	}

private:
	CGameHandler & gameHandler;
	int lastRequestID = 0;
};

TinyH3M::TinyH3MBuilder makeGateMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::HOTA);
	builder
		.hotaVersion(3)
		.size(36, false)
		.name("NK2QuestGate")
		.playerActive(PLAYER)
		.hero(HERO_ANCHOR_POS, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.questGate(GATE_ANCHOR_POS, TinyH3M::TinyH3MBuilder::missionLevel(1));

	return builder;
}

TinyH3M::TinyH3MBuilder makeQuestGuardMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(PLAYER)
		.hero(HERO_ANCHOR_POS, HeroTypeID(0), PLAYER)
		.heroGarrison({{CreatureID(27), 1}})
		.questGuard(GATE_ANCHOR_POS, TinyH3M::TinyH3MBuilder::missionLevel(1));
	return builder;
}

class Nullkiller2_Pathfinding_QuestAction : public NullkillerTest
{
public:
	void revealMapAndEncloseHero(const int3 & openTile = GATE_POS)
	{
		revealMap(PLAYER);

		for(int dx = -1; dx <= 1; ++dx)
		{
			for(int dy = -1; dy <= 1; ++dy)
			{
				const int3 tile = HERO_POS + int3(dx, dy, 0);
				if(tile != HERO_POS && tile != openTile)
					map()->getTile(tile).terrainType = TerrainId(ETerrainId::ROCK);
			}
		}
	}

	std::vector<NK2AI::AIPath> updateAndGetPaths(const int3 & destination, IClient * client = nullptr)
	{
		auto * hero = findHeroByOwner(PLAYER);
		EXPECT_NE(hero, nullptr);
		if(!hero)
			return {};

		gateway = makeGateway(PLAYER, client);
		NK2AI::HeroMap<NK2AI::HeroRole> heroes;
		heroes[hero] = NK2AI::HeroRole::MAIN;
		NK2AI::PathfinderSettings settings;
		settings.allowBypassObjects = true;
		gateway->nullkiller->pathfinder->updatePaths(heroes, settings);
		return gateway->nullkiller->pathfinder->getPathInfo(destination);
	}

	std::unique_ptr<NK2AI::AIGateway> gateway;
};
}

TEST_F(Nullkiller2_Pathfinding_QuestAction, addsInitialVisitBeforeCrossingUnopenedGate)
{
	startWithMap(makeGateMap());
	revealMapAndEncloseHero();

	gameState()->actingPlayers.insert(PLAYER);
	GameHandlerTestServer server(gameState(), PLAYER);
	CGameHandler gameHandler(server, gameState());
	GameHandlerClient client(gameHandler);
	const auto paths = updateAndGetPaths(TARGET_POS, &client);
	ASSERT_FALSE(paths.empty()) << "the satisfiable gate should not block planning";

	const auto & path = paths.front();
	const auto actionNode = std::find_if(
		path.nodes.begin(),
		path.nodes.end(),
		[](const NK2AI::AIPathNodeInfo & node)
		{
			return dynamic_cast<const NK2AI::AIPathfinding::QuestAction *>(node.specialAction.get());
		});

	ASSERT_NE(actionNode, path.nodes.end())
		<< "an unopened gate must be visited before planning movement through it";
	EXPECT_FALSE(actionNode->actionIsBlocked);

	NK2AI::Goals::ExecuteHeroChain(path, nullptr).accept(gateway.get());
	const auto remainingPaths = updateAndGetPaths(TARGET_POS, &client);
	ASSERT_FALSE(remainingPaths.empty());
	NK2AI::Goals::ExecuteHeroChain(remainingPaths.front(), nullptr).accept(gateway.get());
	const auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	EXPECT_EQ(hero->visitablePos(), TARGET_POS);
}

TEST_F(Nullkiller2_Pathfinding_QuestAction, rejectsActionAfterQuestObjectWasRemoved)
{
	startWithMap(makeGateMap());

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	auto * gate = findObjectAt(GATE_ANCHOR_POS);
	ASSERT_NE(gate, nullptr);

	NK2AI::AIPathfinding::QuestAction questAction(QuestInfo(gate->id));
	const auto gateway = makeGateway(PLAYER);
	const ObjectInstanceID gateID = gate->id;
	RemoveObject removeObject(gateID, PLAYER);
	gameState()->apply(removeObject);
	ASSERT_EQ(gameState()->getObjInstance(gateID), nullptr);

	EXPECT_THROW(
		questAction.execute(gateway.get(), hero),
		NK2AI::cannotFulfillGoalException);
}

TEST_F(Nullkiller2_Pathfinding_QuestAction, satisfiedQuestGuardDoesNotUseGateInitialVisitAction)
{
	startWithMap(makeQuestGuardMap());
	auto * guard = findObjectAt(GATE_ANCHOR_POS);
	ASSERT_NE(guard, nullptr);
	revealMapAndEncloseHero(guard->visitablePos());
	for(int dx = -1; dx <= 1; ++dx)
	{
		for(int dy = -1; dy <= 1; ++dy)
		{
			const int3 tile = TARGET_POS + int3(dx, dy, 0);
			if(tile != TARGET_POS)
				map()->getTile(tile).terrainType = TerrainId(ETerrainId::ROCK);
		}
	}

	const auto paths = updateAndGetPaths(guard->visitablePos());
	ASSERT_FALSE(paths.empty());
	for(const auto & path : paths)
	{
		EXPECT_FALSE(std::ranges::any_of(path.nodes, [](const NK2AI::AIPathNodeInfo & node)
		{
			return dynamic_cast<const NK2AI::AIPathfinding::QuestAction *>(node.specialAction.get());
		}));
	}

	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);

	NK2AI::Nullkiller::baseGraph = std::make_unique<NK2AI::ObjectGraph>();
	NK2AI::Nullkiller::baseGraph->addObject(hero);
	NK2AI::Nullkiller::baseGraph->addObject(guard);
	NK2AI::Nullkiller::baseGraph->registerJunction(TARGET_POS);
	NK2AI::Nullkiller::baseGraph->tryAddConnection(hero->visitablePos(), guard->visitablePos(), 1.0f, 0);
	NK2AI::Nullkiller::baseGraph->tryAddConnection(guard->visitablePos(), TARGET_POS, 2.0f, 0);

	NK2AI::GraphPaths graphPaths;
	graphPaths.calculatePaths(hero, gateway->nullkiller.get(), 4);
	std::vector<NK2AI::AIPath> graphRoutes;
	graphPaths.addChainInfo(graphRoutes, TARGET_POS, hero, gateway->nullkiller.get());
	ASSERT_FALSE(graphRoutes.empty());
	for(const auto & path : graphRoutes)
	{
		EXPECT_FALSE(std::ranges::any_of(path.nodes, [](const NK2AI::AIPathNodeInfo & node)
		{
			return dynamic_cast<const NK2AI::AIPathfinding::QuestAction *>(node.specialAction.get());
		}));
	}
}
