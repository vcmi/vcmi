/*
 * ArmyManagementTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include <gtest/gtest.h>

#include "CGameHandler.h"
#include "../../server/activities/Activity.h"
#include "../../server/activities/ActivityProcessor.h"

#include "mock/GameHandlerTestServer.h"
#include "mock/TinyH3MBuilder.h"
#include "mock/TinyMapGameTest.h"

#include "lib/CPlayerState.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGCreature.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGPandoraBox.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapObjects/MiscObjects.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"
#include "lib/networkPacks/StackLocation.h"

namespace
{

constexpr ui8 swapStacks = 1;
constexpr ui8 mergeStacks = 2;
constexpr ui8 splitStacks = 3;

const PlayerColor red(0);
const CreatureID pikeman(0);
const CreatureID halberdier(1);
const CreatureID archangel(13);

class ArmyManagementTest : public TinyMapGameTest
{
protected:
	std::unique_ptr<GameHandlerTestServer> server;
	std::unique_ptr<CGameHandler> gameHandler;

	void start(TinyH3M::TinyH3MBuilder builder)
	{
		startWithMap(std::move(builder));
		server = std::make_unique<GameHandlerTestServer>(gameState(), red);
		gameHandler = std::make_unique<CGameHandler>(*server, gameState());
	}

	void TearDown() override
	{
		gameHandler.reset();
		server.reset();
		TinyMapGameTest::TearDown();
	}
};

TinyH3M::TinyH3MBuilder twoHeroesMap(std::vector<std::pair<CreatureID, uint16_t>> first, std::vector<std::pair<CreatureID, uint16_t>> second)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.hero(int3(5, 5, 0), HeroTypeID(0), red)
		.heroGarrison(std::move(first))
		.hero(int3(8, 5, 0), HeroTypeID(1), red)
		.heroGarrison(std::move(second));
	return builder;
}

}

TEST_F(ArmyManagementTest, rebalanceCanShrinkDestinationStack)
{
	start(twoHeroesMap({{pikeman, 10}}, {{pikeman, 10}}));
	const auto * left = findHeroAt(int3(5, 5, 0));
	const auto * right = findHeroAt(int3(8, 5, 0));

	EXPECT_TRUE(gameHandler->arrangeStacks(left->id, right->id, splitStacks, SlotID(0), SlotID(0), 4, red));
	EXPECT_EQ(left->getStackCount(SlotID(0)), 16);
	EXPECT_EQ(right->getStackCount(SlotID(0)), 4);

	// Shrinking by one unit must not be read as "move whole stack"
	EXPECT_TRUE(gameHandler->arrangeStacks(left->id, right->id, splitStacks, SlotID(0), SlotID(0), 3, red));
	EXPECT_EQ(left->getStackCount(SlotID(0)), 17);
	EXPECT_EQ(right->getStackCount(SlotID(0)), 3);
}

TEST_F(ArmyManagementTest, stackCannotBeMovedOntoItself)
{
	start(twoHeroesMap({{pikeman, 10}, {pikeman, 5}}, {{halberdier, 1}}));
	const auto * hero = findHeroAt(int3(5, 5, 0));

	EXPECT_FALSE(gameHandler->arrangeStacks(hero->id, hero->id, mergeStacks, SlotID(0), SlotID(0), 0, red));
	EXPECT_FALSE(gameHandler->arrangeStacks(hero->id, hero->id, splitStacks, SlotID(0), SlotID(0), 20, red));
	EXPECT_FALSE(gameHandler->arrangeStacks(hero->id, hero->id, swapStacks, SlotID(0), SlotID(0), 0, red));
	EXPECT_FALSE(gameHandler->bulkMoveArmy(hero->id, hero->id, SlotID(0)));

	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);
	EXPECT_EQ(hero->getStackCount(SlotID(1)), 5);
}

TEST_F(ArmyManagementTest, unitsCanBeAddedToLockedGarrisonButNotTaken)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.hero(int3(5, 5, 0), HeroTypeID(0), red)
		.heroGarrison({{pikeman, 10}, {halberdier, 5}})
		.garrison(int3(10, 10, 0), red, {{pikeman, 50}, {halberdier, 5}}, false);
	start(std::move(builder));

	const auto * hero = findHeroAt(int3(5, 5, 0));
	const auto * garrison = findFirst<CGGarrison>();
	ASSERT_NE(garrison, nullptr);
	ASSERT_FALSE(garrison->removableUnits);

	EXPECT_FALSE(gameHandler->arrangeStacks(hero->id, garrison->id, splitStacks, SlotID(0), SlotID(0), 20, red));
	EXPECT_FALSE(gameHandler->bulkMoveArmy(garrison->id, hero->id, SlotID(0)));
	EXPECT_FALSE(gameHandler->disbandCreature(garrison->id, SlotID(1)));
	EXPECT_EQ(garrison->getStackCount(SlotID(0)), 50);
	EXPECT_EQ(garrison->getStackCount(SlotID(1)), 5);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);

	EXPECT_TRUE(gameHandler->arrangeStacks(hero->id, garrison->id, splitStacks, SlotID(0), SlotID(0), 55, red));
	EXPECT_EQ(garrison->getStackCount(SlotID(0)), 55);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 5);
}

TEST_F(ArmyManagementTest, upgradeRequiresAvailableUpgrade)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.town(int3(10, 10, 0), FactionID(0), red);
	start(std::move(builder));

	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	town->creatures = {{0, {pikeman, halberdier}}};
	gameHandler->insertNewStack(StackLocation(town->id, SlotID(0)), pikeman.toCreature(), 10);
	grantResources(red, GameResID::GOLD, 100000);

	EXPECT_FALSE(gameHandler->upgradeCreature(town->id, SlotID(0), archangel));
	EXPECT_EQ(town->getCreature(SlotID(0)), pikeman.toCreature());

	EXPECT_TRUE(gameHandler->upgradeCreature(town->id, SlotID(0), halberdier));
	EXPECT_EQ(town->getCreature(SlotID(0)), halberdier.toCreature());
}

TEST_F(ArmyManagementTest, armyRequestsOnObjectWithoutArmyAreRejected)
{
	TinyH3M::TinyH3MBuilder builder = twoHeroesMap({{pikeman, 10}}, {{pikeman, 10}});
	builder.shipyard(int3(12, 12, 0), red);
	start(std::move(builder));
	gameState()->actingPlayers = {red};

	const auto * hero = findHeroAt(int3(5, 5, 0));
	const auto * shipyard = findFirst<CGShipyard>();
	ASSERT_NE(shipyard, nullptr);
	const ObjectInstanceID missingObject(1000);

	EXPECT_FALSE(gameHandler->disbandCreature(shipyard->id, SlotID(0)));
	EXPECT_FALSE(gameHandler->upgradeCreature(shipyard->id, SlotID(0), halberdier));
	EXPECT_FALSE(gameHandler->bulkMoveArmy(hero->id, shipyard->id, SlotID(0)));

	BulkSplitStack split(shipyard->id, SlotID(0), 1);
	BulkMergeStacks merge(shipyard->id, SlotID(0));
	BulkSplitAndRebalanceStack rebalance(shipyard->id, SlotID(0));
	BulkMoveArmy move(shipyard->id, hero->id, SlotID(0));
	for(CPackForServer * pack : std::initializer_list<CPackForServer *>{&split, &merge, &rebalance, &move})
	{
		pack->player = red;
		EXPECT_NO_THROW(gameHandler->handleReceivedPack(GameConnectionID::FIRST_CONNECTION, *pack));
	}
	EXPECT_FALSE(gameHandler->bulkMoveArmy(hero->id, missingObject, SlotID(0)));
	EXPECT_FALSE(gameHandler->isAllowedExchange(red, hero->id, missingObject));
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);
}

TEST_F(ArmyManagementTest, rejectedRecruitmentCostsNothing)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.town(int3(10, 10, 0), FactionID(0), red);
	start(std::move(builder));

	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	town->creatures = {{5, {CreatureID(CreatureID::BALLISTA)}}};
	grantResources(red, GameResID::GOLD, 100000);
	const auto goldBefore = gameState()->getPlayerState(red)->resources[GameResID::GOLD];

	// war machine without hero to receive it
	EXPECT_FALSE(gameHandler->recruitCreatures(town->id, town->id, CreatureID::BALLISTA, 1, 0, red));
	EXPECT_EQ(gameState()->getPlayerState(red)->resources[GameResID::GOLD], goldBefore);
	EXPECT_EQ(town->creatures.at(0).first, 5);

	EXPECT_FALSE(gameHandler->recruitCreatures(town->id, town->id, CreatureID(100000), 1, -1, red));
}

TEST_F(ArmyManagementTest, invalidRebalanceIsNotApplied)
{
	start(twoHeroesMap({{pikeman, 10}, {halberdier, 5}}, {{pikeman, 10}}));
	const auto * hero = findHeroAt(int3(5, 5, 0));

	const auto rebalance = [hero](SlotID src, SlotID dst, TQuantity count)
	{
		RebalanceStacks rs;
		rs.srcArmy = hero->id;
		rs.dstArmy = hero->id;
		rs.srcSlot = src;
		rs.dstSlot = dst;
		rs.count = count;
		return rs;
	};

	auto zero = rebalance(SlotID(0), SlotID(3), 0);
	auto negative = rebalance(SlotID(0), SlotID(3), -4);
	auto tooMany = rebalance(SlotID(0), SlotID(3), 11);
	auto ontoItself = rebalance(SlotID(0), SlotID(0), 10);
	auto differentCreature = rebalance(SlotID(0), SlotID(1), 4);
	EXPECT_THROW(gameState()->apply(zero), std::runtime_error);
	EXPECT_THROW(gameState()->apply(negative), std::runtime_error);
	EXPECT_THROW(gameState()->apply(tooMany), std::runtime_error);
	EXPECT_THROW(gameState()->apply(ontoItself), std::runtime_error);
	EXPECT_THROW(gameState()->apply(differentCreature), std::runtime_error);

	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);
	EXPECT_EQ(hero->getStackCount(SlotID(1)), 5);
	EXPECT_FALSE(hero->hasStackAtSlot(SlotID(3)));
}

TEST_F(ArmyManagementTest, smartRebalanceOfTwoUnits)
{
	start(twoHeroesMap({{pikeman, 2}}, {{pikeman, 10}}));
	const auto * hero = findHeroAt(int3(5, 5, 0));

	EXPECT_TRUE(gameHandler->bulkSplitAndRebalanceStack(SlotID(0), hero->id));
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 1);
	EXPECT_EQ(hero->stacksCount(), 2);
}

TEST_F(ArmyManagementTest, neutralStackSplitFollowsHotaMode)
{
	TinyH3M::TinyH3MBuilder builder = twoHeroesMap({{archangel, 5}}, {{pikeman, 10}});
	// position where H3 random adjustment would remove one stack
	builder.monster(int3(15, 10, 0), pikeman, 50);
	start(std::move(builder));

	const auto * hero = findHeroAt(int3(5, 5, 0));
	auto * monster = findFirst<CGCreature>();
	ASSERT_NE(monster, nullptr);

	monster->stacksCount = -3; // average
	const int average = monster->getNumberOfStacks(hero);
	ASSERT_GT(average, 1);
	ASSERT_LT(average, GameConstants::ARMY_SIZE);

	monster->stacksCount = 0; // one more
	EXPECT_EQ(monster->getNumberOfStacks(hero), average + 1);

	monster->stacksCount = -2; // one less
	EXPECT_EQ(monster->getNumberOfStacks(hero), average - 1);

	monster->stacksCount = 100; // explicit amount is limited by army size
	EXPECT_EQ(monster->getNumberOfStacks(hero), GameConstants::ARMY_SIZE);
}

TEST_F(ArmyManagementTest, takingMissingCreaturesChangesNothing)
{
	start(twoHeroesMap({{pikeman, 10}, {halberdier, 5}}, {{pikeman, 10}}));
	const auto * hero = findHeroAt(int3(5, 5, 0));

	gameHandler->takeCreatures(hero->id, {CStackBasicDescriptor(pikeman, 20)}, false);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);

	// Repeated entries of the same creature are added up
	gameHandler->takeCreatures(hero->id, {CStackBasicDescriptor(pikeman, 6), CStackBasicDescriptor(pikeman, 6)}, false);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);

	// Last units of the hero are kept unless removal is forced
	gameHandler->takeCreatures(hero->id, {CStackBasicDescriptor(pikeman, 10), CStackBasicDescriptor(halberdier, 5)}, false);
	EXPECT_EQ(hero->stacksCount(), 2);

	gameHandler->takeCreatures(hero->id, {CStackBasicDescriptor(pikeman, 4)}, false);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 6);
}

TEST_F(ArmyManagementTest, rewardWithoutUnitsIsSkipped)
{
	TinyH3M::TinyH3MBuilder builder = twoHeroesMap({{pikeman, 10}}, {{pikeman, 10}});
	builder.pandora(int3(6, 5, 0), {{halberdier, 0}, {archangel, 2}});
	start(std::move(builder));

	auto * hero = findHeroAt(int3(5, 5, 0));
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(pandora, nullptr);

	gameHandler->objectVisited(pandora, hero);
	auto dialog = gameHandler->activities->topActivity(red);
	ASSERT_NE(dialog, nullptr);
	ASSERT_EQ(gameHandler->activities->submitReply(dialog->getActiveQuestionID(), red, 1), ReplyOutcome::Accepted);

	EXPECT_EQ(hero->stacksCount(), 2);
	EXPECT_EQ(hero->getSlotFor(halberdier.toCreature()), hero->getFreeSlot());
	EXPECT_EQ(hero->getStackCount(hero->getSlotFor(archangel.toCreature())), 2);
}

TEST_F(ArmyManagementTest, marketRejectsTradeModeItDoesNotSupport)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.town(int3(10, 10, 0), FactionID(0), red);
	start(std::move(builder));

	auto * town = findFirst<CGTownInstance>();
	ASSERT_NE(town, nullptr);
	ASSERT_FALSE(town->allowsTrade(EMarketMode::CREATURE_UNDEAD));
	gameHandler->insertNewStack(StackLocation(town->id, SlotID(0)), pikeman.toCreature(), 10);
	gameState()->actingPlayers = {red};

	TradeOnMarketplace pack;
	pack.player = red;
	pack.marketId = town->id;
	pack.mode = EMarketMode::CREATURE_UNDEAD;
	pack.r1 = {TradeItemSell(SlotID(0))};
	gameHandler->handleReceivedPack(GameConnectionID::FIRST_CONNECTION, pack);

	EXPECT_EQ(town->getCreature(SlotID(0)), pikeman.toCreature());
}

TEST_F(ArmyManagementTest, armyWithoutOwnerCanNotBeRearrangedOutsideGarrisonWindow)
{
	TinyH3M::TinyH3MBuilder builder = twoHeroesMap({{pikeman, 10}}, {{pikeman, 10}});
	builder.monster(int3(20, 20, 0), halberdier, 30)
		.pandora(int3(25, 25, 0));
	start(std::move(builder));
	gameState()->actingPlayers = {red};

	const auto * monster = findFirst<CGCreature>();
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(monster, nullptr);
	ASSERT_NE(pandora, nullptr);
	ASSERT_TRUE(pandora->setCreature(SlotID(0), halberdier, 5));
	ASSERT_TRUE(pandora->setCreature(SlotID(1), halberdier, 7));

	EXPECT_FALSE(gameHandler->arrangeStacks(monster->id, monster->id, swapStacks, SlotID(0), SlotID(3), 0, red));
	EXPECT_FALSE(gameHandler->arrangeStacks(pandora->id, pandora->id, mergeStacks, SlotID(0), SlotID(1), 0, red));
	EXPECT_FALSE(gameHandler->arrangeStacks(pandora->id, pandora->id, splitStacks, SlotID(1), SlotID(2), 3, red));

	EXPECT_EQ(monster->getStackCount(SlotID(0)), 30);
	EXPECT_EQ(pandora->getStackCount(SlotID(0)), 5);
	EXPECT_EQ(pandora->getStackCount(SlotID(1)), 7);
}

TEST_F(ArmyManagementTest, unitsLeftByRewardCanBeTakenInGarrisonWindow)
{
	TinyH3M::TinyH3MBuilder builder = twoHeroesMap({{pikeman, 10}}, {{pikeman, 10}});
	builder.pandora(int3(6, 5, 0));
	start(std::move(builder));

	const auto * hero = findHeroAt(int3(5, 5, 0));
	auto * pandora = findFirst<CGPandoraBox>();
	ASSERT_NE(pandora, nullptr);
	ASSERT_TRUE(pandora->setCreature(SlotID(0), halberdier, 10));
	ASSERT_TRUE(pandora->setCreature(SlotID(1), archangel, 2));
	gameHandler->showGarrisonDialog(pandora->id, hero->id, MetaString());

	EXPECT_TRUE(gameHandler->arrangeStacks(pandora->id, hero->id, splitStacks, SlotID(0), SlotID(3), 4, red));
	EXPECT_EQ(hero->getStackCount(SlotID(3)), 4);
	EXPECT_EQ(pandora->getStackCount(SlotID(0)), 6);

	BulkMoveArmy moveAll(pandora->id, hero->id, SlotID(0));
	moveAll.player = red;
	gameHandler->handleReceivedPack(GameConnectionID::FIRST_CONNECTION, moveAll);
	EXPECT_EQ(pandora->stacksCount(), 0);
	EXPECT_EQ(hero->getStackCount(SlotID(3)), 10);
	EXPECT_EQ(hero->getStackCount(hero->getSlotFor(archangel.toCreature())), 2);
}

TEST_F(ArmyManagementTest, alliedGarrisonOnlyAcceptsUnits)
{
	const PlayerColor blue(1);
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(red)
		.playerActive(blue)
		.hero(int3(5, 5, 0), HeroTypeID(0), red)
		.heroGarrison({{pikeman, 10}, {halberdier, 5}})
		.garrison(int3(10, 10, 0), blue, {{pikeman, 20}}, true);
	start(std::move(builder));

	const TeamID team = gameState()->players.at(red).team;
	gameState()->players.at(blue).team = team;
	gameState()->teams.at(team).players.insert(blue);
	ASSERT_EQ(gameState()->getPlayerRelations(red, blue), PlayerRelations::ALLIES);

	const auto * hero = findHeroAt(int3(5, 5, 0));
	const auto * garrison = findFirst<CGGarrison>();
	ASSERT_NE(garrison, nullptr);
	gameHandler->showGarrisonDialog(garrison->id, hero->id, MetaString());

	EXPECT_FALSE(gameHandler->arrangeStacks(hero->id, garrison->id, splitStacks, SlotID(0), SlotID(0), 15, red));
	EXPECT_FALSE(gameHandler->arrangeStacks(garrison->id, hero->id, swapStacks, SlotID(0), SlotID(4), 0, red));
	BulkMoveArmy moveAll(garrison->id, hero->id, SlotID(0));
	moveAll.player = red;
	gameHandler->handleReceivedPack(GameConnectionID::FIRST_CONNECTION, moveAll);
	EXPECT_EQ(garrison->getStackCount(SlotID(0)), 20);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 10);

	EXPECT_TRUE(gameHandler->arrangeStacks(hero->id, garrison->id, splitStacks, SlotID(0), SlotID(0), 25, red));
	EXPECT_EQ(garrison->getStackCount(SlotID(0)), 25);
	EXPECT_EQ(hero->getStackCount(SlotID(0)), 5);
}

TEST_F(ArmyManagementTest, mapStackWithoutUnitsIsNotLoaded)
{
	TinyH3M::TinyH3MBuilder builder = twoHeroesMap({{pikeman, 10}}, {{pikeman, 10}});
	builder.garrison(int3(10, 10, 0), red, {{pikeman, 0}, {halberdier, 5}}, true);
	start(std::move(builder));

	const auto * garrison = findFirst<CGGarrison>();
	ASSERT_NE(garrison, nullptr);
	EXPECT_FALSE(garrison->hasStackAtSlot(SlotID(0)));
	EXPECT_EQ(garrison->getStackCount(SlotID(1)), 5);
}
