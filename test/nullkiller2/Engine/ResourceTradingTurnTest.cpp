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

#include "mock/TinyH3MBuilder.h"
#include "quest/QuestTest.h"

#include "lib/CPlayerState.h"
#include "lib/callback/CCallback.h"
#include "lib/callback/IClient.h"
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

class ApplyingClient : public IClient
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
		if(const auto * trade = dynamic_cast<const TradeOnMarketplace *>(&request))
			applyTrade(*trade, player);
		else if(const auto * recruit = dynamic_cast<const RecruitCreatures *>(&request))
			applyRecruitment(*recruit, player);

		return ++lastRequestID;
	}

	void setGameState(CGameState * value)
	{
		gameState = value;
	}

	int getMarketplaceTrades() const
	{
		return marketplaceTrades;
	}

	int getTradingPhases() const
	{
		return tradingPhases;
	}

	int getRecruitmentRequests() const
	{
		return recruitmentRequests;
	}

private:
	void applyTrade(const TradeOnMarketplace & request, PlayerColor player)
	{
		ASSERT_NE(gameState, nullptr);
		ASSERT_EQ(request.mode, EMarketMode::RESOURCE_RESOURCE);

		if(!tradingPhaseActive)
		{
			++tradingPhases;
			tradingPhaseActive = true;
		}

		const auto * market = gameState->getMarket(request.marketId);
		ASSERT_NE(market, nullptr);
		ASSERT_EQ(request.r1.size(), request.r2.size());
		ASSERT_EQ(request.r1.size(), request.val.size());

		auto & resources = gameState->players.at(player).resources;
		for(size_t index = 0; index < request.r1.size(); ++index)
		{
			const auto soldResource = request.r1[index].as<GameResID>();
			const auto boughtResource = request.r2[index].as<GameResID>();
			int givenPerUnit = 0;
			int receivedPerUnit = 0;
			market->getOffer(
				soldResource,
				boughtResource,
				givenPerUnit,
				receivedPerUnit,
				EMarketMode::RESOURCE_RESOURCE);

			ASSERT_GT(givenPerUnit, 0);
			ASSERT_GT(receivedPerUnit, 0);
			ASSERT_EQ(request.val[index] % givenPerUnit, 0);

			resources[soldResource] -= request.val[index];
			resources[boughtResource] += request.val[index] / givenPerUnit * receivedPerUnit;
		}

		++marketplaceTrades;
	}

	void applyRecruitment(const RecruitCreatures & request, PlayerColor player)
	{
		ASSERT_NE(gameState, nullptr);
		auto * town = gameState->getTown(request.tid);
		ASSERT_NE(town, nullptr);
		ASSERT_GE(request.level, 0);
		ASSERT_LT(request.level, static_cast<int>(town->creatures.size()));
		ASSERT_GE(town->creatures[request.level].first, request.amount);

		town->creatures[request.level].first -= request.amount;
		gameState->players.at(player).resources -=
			request.crid.toCreature()->getFullRecruitCost() * request.amount;
		tradingPhaseActive = false;
		++recruitmentRequests;
	}

	CGameState * gameState = nullptr;
	bool tradingPhaseActive = false;
	int lastRequestID = 0;
	int marketplaceTrades = 0;
	int tradingPhases = 0;
	int recruitmentRequests = 0;
};

class ResourceTradingTurnTest : public QuestTest
{
public:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.randomTown({9, 5, 0}, PLAYER)
			.hero({25, 25, 0}, HeroTypeID(0), PLAYER);

		startWithMap(std::move(builder));
	}

	void prepareTradingCycle(CGTownInstance & town)
	{
		NewStructures structures;
		structures.tid = town.id;
		for(const auto & building : town.getTown()->buildings)
			structures.bid.insert(building.first);
		gameState->apply(structures);

		for(auto & creatureLevel : town.creatures)
		{
			if(!creatureLevel.second.empty())
				creatureLevel.first = 100;
		}

		auto & resources = gameState->players.at(PLAYER).resources;
		for(int resource = 0; resource < GameConstants::RESOURCE_QUANTITY; ++resource)
			resources[resource] = resource == GameResID::GOLD ? 0 : 1000000;

		revealMap(PLAYER);

		CSimpleArmy emptyArmy;
		gameState->heroesPool->setHeroForPlayer(
			PLAYER,
			TavernHeroSlot::NATIVE,
			HeroTypeID::NONE,
			emptyArmy,
			TavernSlotRole::NONE,
			false);
		gameState->heroesPool->setHeroForPlayer(
			PLAYER,
			TavernHeroSlot::RANDOM,
			HeroTypeID::NONE,
			emptyArmy,
			TavernSlotRole::NONE,
			false);
	}

protected:
	ApplyingClient client;
};
}

TEST_F(ResourceTradingTurnTest, tradesForArmyOnlyOncePerTurn)
{
	startGame();

	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	auto * hero = findHeroByOwner(PLAYER);
	ASSERT_NE(hero, nullptr);
	prepareTradingCycle(*town);
	SetMovePoints stopHero(hero->id, 0);
	gameState->apply(stopHero);
	client.setGameState(gameState.get());

	auto gateway = std::make_unique<NK2AI::AIGateway>();
	gateway->initGameInterface(std::shared_ptr<Environment>(), makeCallback(PLAYER, &client));

	gateway->nullkiller->makeTurn();

	EXPECT_GT(client.getRecruitmentRequests(), 0)
		<< "the fixture must consume traded gold by buying army";
	EXPECT_GT(client.getMarketplaceTrades(), 0)
		<< "the fixture must fund army purchases through a marketplace";
	EXPECT_EQ(client.getTradingPhases(), 1)
		<< "army purchases must not reopen the turn's resource-trading budget";
}
