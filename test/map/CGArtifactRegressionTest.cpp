/*
 * CGArtifactRegressionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../mock/TinyMapGameTest.h"
#include "../mock/GameHandlerTestServer.h"

#include "CGameHandler.h"
#include "GameLibrary.h"

#include "GameSettings.h"
#include "callback/EditorCallback.h"
#include "entities/artifact/ArtifactUtils.h"
#include "entities/artifact/CArtifact.h"
#include "gameState/CGameState.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapping/CMap.h"
#include "mapObjects/MiscObjects.h"

TEST(CGArtifactRegression, RandomArtifactInstanceLookupIsSafe)
{
	EditorCallback cb(nullptr);
	CMap map(&cb);
	cb.setMap(&map);
	CGArtifact randomArtifact(&cb);
	randomArtifact.ID = Obj::RANDOM_ART;

	const CArtifactInstance * instance = nullptr;
	EXPECT_NO_THROW(instance = randomArtifact.getArtifactInstance());
	EXPECT_EQ(instance, nullptr);
}

class ArtifactCombinationTest : public TinyMapGameTest
{
protected:
	Services * gameServices() override { return LIBRARY; }
};

// Cloak of the Undead King (130) is combined from Amulet of the Undertaker (54), Vampire's Cowl (55), Dead Man's Boots (56)
TEST_F(ArtifactCombinationTest, DisabledCombinationCanNotBeAssembled)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PlayerColor(0))
		.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
		.heroEquipped({
			{ArtifactPosition::NECK, ArtifactID(54)},
			{ArtifactPosition::SHOULDERS, ArtifactID(55)},
			{ArtifactPosition::FEET, ArtifactID(56)}});
	startWithMap(std::move(builder));
	const auto * hero = findHeroByOwner(PlayerColor(0));
	ASSERT_NE(hero, nullptr);

	auto possible = ArtifactUtils::assemblyPossibilities(gameState()->getSettings(), hero, ArtifactID(56));
	ASSERT_EQ(possible.size(), 1);
	EXPECT_EQ(possible.front()->getId(), ArtifactID(130));

	GameSettings settings;
	JsonNode disabled;
	disabled.Vector().emplace_back("cloakOfTheUndeadKing");
	settings.addOverride(EGameSettings::ARTIFACTS_DISABLED_COMBINATIONS, disabled);
	EXPECT_TRUE(ArtifactUtils::assemblyPossibilities(settings, hero, ArtifactID(56)).empty());
}

TEST_F(ArtifactCombinationTest, DisabledCombinationCanNotBeDisassembled)
{
	TinyH3M::TinyH3MBuilder builder(EMapFormat::SOD);
	builder.size(36, false)
		.playerActive(PlayerColor(0))
		.hero({5, 5, 0}, HeroTypeID(0), PlayerColor(0))
		.heroEquipped({{ArtifactPosition::SHOULDERS, ArtifactID(130)}});
	startWithMap(std::move(builder));
	const auto * hero = findHeroByOwner(PlayerColor(0));
	ASSERT_NE(hero, nullptr);
	ASSERT_EQ(hero->getArt(ArtifactPosition::SHOULDERS)->getTypeId(), ArtifactID(130));

	JsonNode disabled;
	disabled.Vector().emplace_back("cloakOfTheUndeadKing");
	gameState()->getMap().overrideGameSetting(EGameSettings::ARTIFACTS_DISABLED_COMBINATIONS, disabled);

	GameHandlerTestServer server(gameState());
	CGameHandler gameHandler(server, gameState());
	EXPECT_FALSE(gameHandler.assembleArtifacts(hero->id, ArtifactPosition::SHOULDERS, false, ArtifactID()));
	EXPECT_EQ(hero->getArt(ArtifactPosition::SHOULDERS)->getTypeId(), ArtifactID(130));
}
