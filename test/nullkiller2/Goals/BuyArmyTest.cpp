/*
 * BuyArmyTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "AI/Nullkiller2/AIGateway.h"
#include "AI/Nullkiller2/Goals/BuyArmy.h"

#include "mock/TinyH3MBuilder.h"
#include "quest/QuestTest.h"

#include "lib/CPlayerState.h"
#include "lib/callback/CCallback.h"
#include "lib/callback/IClient.h"
#include "lib/gameState/CGameState.h"
#include "lib/mapObjects/CGHeroInstance.h"
#include "lib/mapObjects/CGTownInstance.h"
#include "lib/mapping/CMap.h"
#include "lib/networkPacks/PacksForClient.h"
#include "lib/networkPacks/PacksForServer.h"

namespace
{
const PlayerColor PLAYER = PlayerColor(0);
const PlayerColor FOREIGN_PLAYER = PlayerColor(1);

void fillFullArmy(CCreatureSet & army)
{
	const std::array<CreatureID, GameConstants::ARMY_SIZE> creatures = {
		CreatureID(CreatureID::ARCHER),
		CreatureID(0),
		CreatureID(1),
		CreatureID(4),
		CreatureID(5),
		CreatureID(6),
		CreatureID(7)
	};

	for(size_t index = 0; index < creatures.size(); ++index)
		ASSERT_TRUE(army.setCreature(SlotID(index), creatures[index], 1));
}

class RecordingClient : public IClient
{
public:
	std::optional<BattleAction> makeSurrenderRetreatDecision(
		PlayerColor,
		const BattleID &,
		const BattleStateInfoForRetreat &) override
	{
		return std::nullopt;
	}

	int sendRequest(const CPackForServer & request, PlayerColor, bool) override
	{
		if(dynamic_cast<const MoveHero *>(&request))
			++moveHeroRequests;
		return ++lastRequestID;
	}

	int getMoveHeroRequests() const
	{
		return moveHeroRequests;
	}

private:
	int lastRequestID = 0;
	int moveHeroRequests = 0;
};

class Nullkiller2_Goals_BuyArmyTest : public QuestTest
{
public:
	void startGame()
	{
		TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
		builder
			.size(36, false)
			.playerActive(PLAYER)
			.playerActive(FOREIGN_PLAYER)
			.randomTown({9, 5, 0}, PLAYER)
			.hero({25, 25, 0}, HeroTypeID(0), FOREIGN_PLAYER);

		startWithMap(std::move(builder));
	}

	std::unique_ptr<NK2AI::AIGateway> makeGateway()
	{
		auto gateway = std::make_unique<NK2AI::AIGateway>();
		gateway->initGameInterface(std::shared_ptr<Environment>(), makeCallback(PLAYER, &client));
		return gateway;
	}

protected:
	RecordingClient client;
};
}

TEST(Nullkiller2_Goals_BuyArmy, fullArmyDoesNotNeedFreeSlotForExistingCreature)
{
	CGHeroInstance army(nullptr);
	fillFullArmy(army);

	ASSERT_EQ(army.stacksCount(), GameConstants::ARMY_SIZE);

	EXPECT_FALSE(NK2AI::Goals::BuyArmy::needsFreeSlotToRecruit(
		&army,
		CreatureID(CreatureID::ARCHER)))
		<< "recruiting an existing creature type can merge without dismissing a stack";

	EXPECT_TRUE(NK2AI::Goals::BuyArmy::needsFreeSlotToRecruit(
		&army,
		CreatureID(8)))
		<< "a full army still needs a free slot for a missing creature type";
}

TEST(Nullkiller2_Goals_BuyArmy, partialArmyDoesNotNeedFreeSlotForMissingCreature)
{
	CGHeroInstance army(nullptr);
	ASSERT_TRUE(army.setCreature(SlotID(0), CreatureID(CreatureID::ARCHER), 1));

	EXPECT_FALSE(NK2AI::Goals::BuyArmy::needsFreeSlotToRecruit(
		&army,
		CreatureID(8)));
}

TEST_F(Nullkiller2_Goals_BuyArmyTest, doesNotMoveForeignTownVisitor)
{
	startGame();

	auto * town = findFirst<CGTownInstance>();
	auto * foreignHero = findHeroByOwner(FOREIGN_PLAYER);
	ASSERT_NE(town, nullptr);
	ASSERT_NE(foreignHero, nullptr);

	ChangeObjPos moveHero;
	moveHero.objid = foreignHero->id;
	moveHero.nPos = town->visitablePos();
	moveHero.initiator = FOREIGN_PLAYER;
	gameState->apply(moveHero);
	town->setVisitingHero(foreignHero);

	ASSERT_FALSE(town->creatures.empty());
	ASSERT_FALSE(town->creatures.front().second.empty());
	town->creatures.front().first = 10;
	gameState->players.at(PLAYER).resources += 1000000;

	const auto gateway = makeGateway();
	NK2AI::Goals::BuyArmy(town, 1).accept(gateway.get());

	EXPECT_EQ(client.getMoveHeroRequests(), 0)
		<< "BuyArmy must not issue movement requests for another player's hero";
}
