/*
 * HeroRecruitmentTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/CPlayerState.h"

/**
 * Test fixture for hero recruitment netpack tests.
 * Inherits game setup and state management from GameStateTest.
 */
class HeroRecruitmentTest : public GameStateTest
{
};

TEST_F(HeroRecruitmentTest, recruitedHeroGetsFreshInstanceNameWhenPoolEntryConflicts)
{
	startTestGame();

	PlayerColor playerColor = PlayerColor::NEUTRAL;

	for (int playerIndex = 0; playerIndex < PlayerColor::PLAYER_LIMIT_I; ++playerIndex)
	{
		const auto currentPlayer = PlayerColor(playerIndex);
		if (gameState->getPlayerState(currentPlayer))
		{
			playerColor = currentPlayer;
			break;
		}
	}

	ASSERT_TRUE(playerColor.isValidPlayer()) << "Test map must provide a recruitable hero";
	auto * playerState = gameState->getPlayerState(playerColor);
	ASSERT_NE(playerState, nullptr);

	const auto heroesInPool = map->getHeroesInPool();
	ASSERT_FALSE(heroesInPool.empty()) << "Test map must provide a hero in the recruitment pool";
	auto * pooledHero = map->tryGetFromHeroPool(heroesInPool.front());
	ASSERT_NE(pooledHero, nullptr);

	auto objects = map->getObjects();
	ASSERT_FALSE(objects.empty()) << "Test map must contain at least one object";
	const std::string staleInstanceName = objects.front()->instanceName;
	pooledHero->instanceName = staleInstanceName;

	HeroRecruited pack;
	pack.tid = ObjectInstanceID();
	pack.hid = pooledHero->getHeroTypeID();
	pack.player = playerColor;
	pack.tile = int3(5, 5, 0);

	ASSERT_NO_THROW(gameEventCallback->sendAndApply(pack));

	auto * recruitedHero = dynamic_cast<CGHeroInstance *>(map->getObject(pooledHero->id));
	ASSERT_NE(recruitedHero, nullptr);
	EXPECT_NE(recruitedHero->instanceName, staleInstanceName);
	EXPECT_EQ(map->instanceNames.count(recruitedHero->instanceName), 1);
}
