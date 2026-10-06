/*
 * HeroMovementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/GameHandlerTestServer.h"
#include "../mock/TinyMapGameTest.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/TerrainTile.h"
#include "../../server/CGameHandler.h"

class HeroMovementTest : public TinyMapGameTest
{
protected:
	Services * gameServices() override { return LIBRARY; }
};

/// Two towns that share an entrance tile, as found on some SoD maps. The pathfinder only considers the top town.
TEST_F(HeroMovementTest, sharedEntranceInteractsOnlyWithTopTown)
{
	const PlayerColor player(0);
	const int3 townPos(10, 10, 0);
	const int3 entrance(8, 10, 0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(player)
		.town(townPos, FactionID::CASTLE, PlayerColor::NEUTRAL)
		.town(townPos, FactionID::CASTLE, player)
		.hero(int3(9, 11, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));

	auto towns = findAll<CGTownInstance>();
	ASSERT_EQ(towns.size(), 2);
	auto * neutralTown = towns[0]->getOwner() == player ? towns[1] : towns[0];
	auto * ownTown = towns[0]->getOwner() == player ? towns[0] : towns[1];
	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);

	ASSERT_EQ(neutralTown->visitablePos(), entrance);
	ASSERT_EQ(ownTown->visitablePos(), entrance);
	ASSERT_EQ(map()->getTile(entrance).topVisitableObj(), ownTown->id);
	ASSERT_EQ(hero->visitablePos(), entrance + int3(0, 1, 0));

	if(!neutralTown->stacksCount())
	{
		ASSERT_TRUE(neutralTown->setCreature(SlotID(0), CreatureID(0), 10));
	}
	ASSERT_FALSE(neutralTown->passableFor(player));

	GameHandlerTestServer server(gameState(), player);
	CGameHandler gameHandler(server, gameState());

	ASSERT_TRUE(gameHandler.moveHero(hero->id, hero->convertFromVisitablePos(entrance), EMovementMode::STANDARD, false, player, EPathfindingLayer::LAND));

	EXPECT_EQ(hero->visitablePos(), entrance);
	EXPECT_EQ(ownTown->getVisitingHero(), hero);
	EXPECT_EQ(neutralTown->getVisitingHero(), nullptr);
	EXPECT_EQ(neutralTown->getOwner(), PlayerColor::NEUTRAL);
	EXPECT_EQ(gameState()->getBattle(player), nullptr);
}

/// A hero starting on an entrance shared by two towns of its owner visits only the top town
TEST_F(HeroMovementTest, heroStartingOnSharedEntranceVisitsOnlyTopTown)
{
	const PlayerColor player(0);
	const int3 townPos(10, 10, 0);
	const int3 entrance(8, 10, 0);

	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder
		.size(36, false)
		.playerActive(player)
		.town(townPos, FactionID::CASTLE, player)
		.town(townPos, FactionID::CASTLE, player)
		.hero(int3(9, 10, 0), HeroTypeID(0), player);
	startWithMap(std::move(builder));

	auto * hero = findHeroByOwner(player);
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->visitablePos(), entrance);

	const auto & tile = map()->getTile(entrance);
	const auto * topTown = gameState()->getTown(tile.objectVisitedBy(hero->id));
	ASSERT_NE(topTown, nullptr);

	EXPECT_EQ(hero->getVisitedTown(), topTown);
	for(const auto * town : findAll<CGTownInstance>())
		EXPECT_EQ(town->getVisitingHero(), town == topTown ? hero : nullptr);
}
