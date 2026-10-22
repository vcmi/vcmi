/*
 * ResourceTradingTurnTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"

#include "mock/GameHandlerTestClient.h"
#include "mock/TinyH3MBuilder.h"
#include "nullkiller2/NullkillerTest.h"

#include "lib/CPlayerState.h"
#include "lib/callback/CCallback.h"
#include "lib/constants/NumericConstants.h"
#include "lib/gameState/CGameState.h"
#include "lib/gameState/TavernHeroesPool.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapObjects/army/CSimpleArmy.h"
#include "lib/mapping/CMap.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor ENEMY = PlayerColor(1);

/// Counts contiguous runs of marketplace trades; any other request ends the current run
class TradingPhaseClient : public GameHandlerTestClient
{
public:
	using GameHandlerTestClient::GameHandlerTestClient;

	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override
	{
		if(dynamic_cast<const TradeOnMarketplace *>(&request))
		{
			if(!tradingPhaseActive)
				++tradingPhases;

			tradingPhaseActive = true;
			++marketplaceTrades;
		}
		else
		{
			tradingPhaseActive = false;

			if(dynamic_cast<const RecruitCreatures *>(&request))
				++recruitmentRequests;
		}

		return GameHandlerTestClient::sendRequest(request, player, waitTillRealize);
	}

	int marketplaceTrades = 0;
	int tradingPhases = 0;
	int recruitmentRequests = 0;

private:
	bool tradingPhaseActive = false;
};

/// Drops marketplace trades as a server that rejects them; forwards trades after the limit so a retry loop still ends
class TradeRejectingClient : public GameHandlerTestClient
{
public:
	static constexpr int REJECTION_LIMIT = 10;

	using GameHandlerTestClient::GameHandlerTestClient;

	int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) override
	{
		if(dynamic_cast<const TradeOnMarketplace *>(&request) && rejectedTrades < REJECTION_LIMIT)
		{
			++rejectedTrades;
			return ++lastRejectedRequestID;
		}

		return GameHandlerTestClient::sendRequest(request, player, waitTillRealize);
	}

	int rejectedTrades = 0;

private:
	int lastRejectedRequestID = 0;
};

class ResourceTradingTurnTest : public NullkillerTest
{
public:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.randomTown({9, 5, 0}, PLAYER)
			.hero({25, 25, 0}, HeroTypeID(0), PLAYER)
			.playerActive(ENEMY)
			.randomTown({30, 30, 0}, ENEMY);

		startWithMap(std::move(builder));
	}

	void prepareTradingTurn()
	{
		auto * town = findFirst<CGTownInstance>();
		ASSERT_NE(town, nullptr);
		auto * hero = findHeroByOwner(PLAYER);
		ASSERT_NE(hero, nullptr);
		prepareTradingCycle(*town);
		SetMovePoints stopHero(hero->id, 0);
		gameState()->apply(stopHero);
	}

	void prepareTradingCycle(CGTownInstance & town)
	{
		NewStructures structures;
		structures.tid = town.id;
		for(const auto & building : town.getTown()->buildings)
			structures.bid.insert(building.first);
		gameState()->apply(structures);

		for(auto & creatureLevel : town.creatures)
		{
			if(!creatureLevel.second.empty())
				creatureLevel.first = 100;
		}

		auto & resources = gameState()->players.at(PLAYER).resources;
		for(int resource = 0; resource < GameConstants::RESOURCE_QUANTITY; ++resource)
			resources[resource] = resource == GameResID::GOLD ? 0 : 1000000;

		revealMap(PLAYER);

		CSimpleArmy emptyArmy;
		gameState()->heroesPool->setHeroForPlayer(
			PLAYER,
			TavernHeroSlot::NATIVE,
			HeroTypeID::NONE,
			emptyArmy,
			TavernSlotRole::NONE,
			false);
		gameState()->heroesPool->setHeroForPlayer(
			PLAYER,
			TavernHeroSlot::RANDOM,
			HeroTypeID::NONE,
			emptyArmy,
			TavernSlotRole::NONE,
			false);
	}
};
}

TEST_F(ResourceTradingTurnTest, tradesForArmyOnlyOncePerTurn)
{
	startGame();
	ASSERT_NO_FATAL_FAILURE(prepareTradingTurn());
	TradingPhaseClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	gateway->nullkiller->makeTurn();

	EXPECT_GT(client.recruitmentRequests, 0)
		<< "the fixture must consume traded gold by buying army";
	EXPECT_GT(client.marketplaceTrades, 0)
		<< "the fixture must fund army purchases through a marketplace";
	EXPECT_EQ(client.tradingPhases, 1)
		<< "all resource trades of a turn must happen in a single trading step";
}

TEST_F(ResourceTradingTurnTest, stopsTradingWhenServerRejectsTrade)
{
	startGame();
	ASSERT_NO_FATAL_FAILURE(prepareTradingTurn());
	TradeRejectingClient client(gameState(), PLAYER);
	auto gateway = makeGateway(PLAYER, &client);

	gateway->nullkiller->makeTurn();

	EXPECT_EQ(client.rejectedTrades, 1)
		<< "a rejected trade must not be retried in the same trading step";
}
