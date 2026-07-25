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
