/*
 * GatherArmyBehaviorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Engine/Nullkiller.h"
#include "AI/Nullkiller2/Markers/HeroExchange.h"

#include "mock/GameHandlerTestServer.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "server/CGameHandler.h"

#include "lib/callback/CCallback.h"
#include "lib/callback/IClient.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"
#include "lib/serializer/CMemorySerializer.h"

namespace
{
const PlayerColor PLAYER(0);
const PlayerColor ENEMY(1);

class GameHandlerClient : public IClient
{
public:
	GameHandlerClient(const std::shared_ptr<CGameState> & gameState, PlayerColor player)
		: server(gameState, player)
		, gameHandler(server, gameState)
	{
		gameState->actingPlayers.insert(player);
	}

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
		gameHandler.handleReceivedPack(
			GameConnectionID::FIRST_CONNECTION,
			*serverRequest);
		return lastRequestID;
	}

private:
	GameHandlerTestServer server;
	CGameHandler gameHandler;
	int lastRequestID = 0;
};

TinyH3M::TinyH3MBuilder makeGarrisonUpgradeMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2GarrisonUpgrade")
		.playerActive(PLAYER)
		.playerActive(ENEMY)
		.town({9, 5, 0}, FactionID::CASTLE, PLAYER)
		.hero({5, 5, 0}, HeroTypeID(HeroTypeID::decode("orrin")), PLAYER)
		.town({30, 30, 0}, FactionID::CASTLE, ENEMY);

	return builder;
}

TinyH3M::TinyH3MBuilder makeArmyExchangeMap()
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.name("NK2ArmyExchange")
		.playerActive(PLAYER)
		.playerActive(ENEMY)
		.town({9, 5, 0}, FactionID::CASTLE, PLAYER)
		.hero({5, 5, 0}, HeroTypeID(HeroTypeID::decode("orrin")), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 1000}})
		.hero({6, 5, 0}, HeroTypeID(HeroTypeID::decode("valeska")), PLAYER)
		.heroGarrison({{CreatureID::ARCHER, 100}})
		.town({30, 30, 0}, FactionID::CASTLE, ENEMY);

	return builder;
}

class Nullkiller2_Behaviors_GatherArmyBehavior : public NullkillerTest
{
protected:
	void putHeroInGarrison(const CGHeroInstance & hero, const CGTownInstance & town) const
	{
		ChangeObjPos moveHero;
		moveHero.objid = hero.id;
		moveHero.nPos = town.visitablePos();
		moveHero.initiator = PLAYER;
		gameState()->apply(moveHero);

		SetHeroesInTown setHeroes;
		setHeroes.tid = town.id;
		setHeroes.visiting = ObjectInstanceID::NONE;
		setHeroes.garrison = hero.id;
		gameState()->apply(setHeroes);
	}
};
}

TEST_F(Nullkiller2_Behaviors_GatherArmyBehavior, upgradesPikemenCarriedByGarrisonHero)
{
	startWithMap(makeGarrisonUpgradeMap());

	auto * hero = findHeroByOwner(PLAYER);
	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(hero, nullptr);
	ASSERT_NE(town, nullptr);
	ASSERT_EQ(town->getFactionID(), FactionID::CASTLE);

	const CreatureID pikeman(CreatureID::decode("pikeman"));
	const CreatureID halberdier(CreatureID::decode("halberdier"));
	town->addBuilding(BuildingID::DWELL_LVL_1_UP);
	town->creatures.at(0).second.push_back(halberdier);
	hero->clearSlots();
	ASSERT_TRUE(hero->setCreature(SlotID(0), pikeman, 1000));
	putHeroInGarrison(*hero, *town);
	grantResources(PLAYER, GameResID(GameResID::GOLD), 1000000);

	GameHandlerClient client(gameState(), PLAYER);
	const auto gateway = makeGateway(PLAYER, &client);
	gateway->nullkiller->makeTurn();

	ASSERT_NE(hero->getStackPtr(SlotID(0)), nullptr);
	EXPECT_EQ(hero->getStackPtr(SlotID(0))->getCreatureID(), halberdier);
}

TEST_F(Nullkiller2_Behaviors_GatherArmyBehavior, armyExchangeIsNotEvaluatedAsNewArmyGrowth)
{
	startWithMap(makeArmyExchangeMap());

	const auto gateway = makeGateway(PLAYER);
	gateway->nullkiller->heroManager->update();
	NK2AI::PathfinderSettings settings;
	settings.useHeroChain = true;
	settings.useDimensionDoor = false;
	gateway->nullkiller->pathfinder->updatePaths(
		gateway->nullkiller->getHeroesForPathfinding(),
		settings);

	const auto heroes = gateway->cc->getHeroesInfo();
	ASSERT_EQ(heroes.size(), 2);
	const auto receiver = std::ranges::find_if(heroes, [&](const auto * hero)
	{
		return gateway->nullkiller->heroManager->getHeroRoleOrDefaultInefficient(hero) == NK2AI::HeroRole::MAIN;
	});
	ASSERT_NE(receiver, heroes.end());
	const auto carrierIterator = std::ranges::find_if(heroes, [&](const auto * hero)
	{
		return hero != *receiver;
	});
	ASSERT_NE(carrierIterator, heroes.end());
	const auto * carrier = *carrierIterator;
	const auto paths = gateway->nullkiller->pathfinder->getPathInfo({7, 5, 0});
	const auto carrierPath = std::ranges::find_if(paths, [&](const auto & path)
	{
		return path.targetHero == carrier;
	});
	ASSERT_NE(carrierPath, paths.end());

	const NK2AI::Goals::HeroExchange exchange(*receiver, *carrierPath);
	ASSERT_GT(exchange.getReinforcementArmyStrength(gateway->nullkiller.get()), 0);
	const auto exchangeGoal = NK2AI::Goals::sptr(exchange);
	const auto context = gateway->nullkiller->priorityEvaluator->buildEvaluationContext(exchangeGoal);
	EXPECT_TRUE(context.isExchange);
	EXPECT_EQ(context.armyGrowth, 0)
		<< "moving an existing army must not outrank tasks that create real growth or map progress";
}
