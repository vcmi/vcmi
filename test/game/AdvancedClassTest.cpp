/*
 * AdvancedClassTest.cpp, part of VCMI engine
 *
 * License: GNU General Public License v2.0 or later
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../mock/GameHandlerTestServer.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/AdvancedClass.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../server/CGameHandler.h"

namespace
{
TEST(AdvancedClassTest, ProgressAdvancesOnlyThroughEmptyTiers)
{
	AdvancedClassProgress progress;
	EXPECT_TRUE(progress.valid());
	EXPECT_EQ(progress.nextTier(), 1);
	progress.classes[0] = "advanced-classes:war_01";
	EXPECT_EQ(progress.nextTier(), 2);
	progress.classes[1] = "advanced-classes:war2_01";
	progress.classes[2] = "advanced-classes:war3_01";
	progress.classes[3] = "advanced-classes:war4_01";
	EXPECT_EQ(progress.nextTier(), 5);
	EXPECT_TRUE(progress.valid());
}

TEST(AdvancedClassTest, ProgressRejectsSkippedTiers)
{
	AdvancedClassProgress progress;
	progress.classes[1] = "advanced-classes:war2_01";

	EXPECT_FALSE(progress.valid());
	EXPECT_EQ(progress.nextTier(), 1);
}

TEST(AdvancedClassTest, HybridScoreUsesWeakestSkillGroup)
{
	AdvancedClassDefinition definition;
	definition.skillGroups = {
		{{SecondarySkill(1), {3, 4, 5}}, {SecondarySkill(2), {3, 4, 5}}},
		{{SecondarySkill(3), {2, 3, 4}}}
	};
	const std::map<SecondarySkill, int> mastery = {
		{SecondarySkill(1), 3},
		{SecondarySkill(2), 2},
		{SecondarySkill(3), 2}
	};

	EXPECT_EQ(definition.calculatePoints([&mastery](SecondarySkill skill)
	{
		return mastery.at(skill);
	}), 3);
}

TEST(AdvancedClassTest, ProgressionRequiresSelectedParent)
{
	AdvancedClassProgress progress;
	progress.classes[0] = "advanced-classes:squire";

	AdvancedClassDefinition definition;
	definition.tier = 2;
	definition.parents = {"advanced-classes:squire"};
	EXPECT_TRUE(definition.follows(progress));

	definition.parents = {"advanced-classes:fighter"};
	EXPECT_FALSE(definition.follows(progress));
}

TEST(AdvancedClassTest, AbilityCooldownUsesWholeBattleRounds)
{
	AdvancedClassDefinition definition;
	definition.abilityConfigured = true;
	definition.abilityCooldown = 2;

	EXPECT_TRUE(definition.isAbilityReady(0, -1000));
	EXPECT_FALSE(definition.isAbilityReady(4, 3));
	EXPECT_TRUE(definition.isAbilityReady(5, 3));
}

TEST(AdvancedClassTest, MissingAbilityIsNeverReady)
{
	AdvancedClassDefinition definition;
	definition.abilityCooldown = 2;

	EXPECT_FALSE(definition.isAbilityReady(100, -1000));
	definition.abilityConfigured = true;
	definition.abilityCooldown = -1;
	EXPECT_FALSE(definition.isAbilityReady(100, -1000));
}

TEST(AdvancedClassTest, ProgressSerializationRoundTrip)
{
	AdvancedClassProgress progress;
	progress.classes = {"test:path1", "test:path2", "test:path3", "test:path4"};

	CMemorySerializer serializer;
	serializer.oser & progress;
	AdvancedClassProgress restored;
	serializer.iser & restored;

	EXPECT_EQ(restored.classes, progress.classes);
	EXPECT_TRUE(serializer.isFullyRead());
}

TEST(AdvancedClassTest, BattleCooldownSerializationRoundTrip)
{
	SideInBattle side(nullptr);
	side.classAbilityLastUse = {3, 7};

	CMemorySerializer serializer;
	serializer.oser & side;
	SideInBattle restored(nullptr);
	serializer.iser & restored;

	EXPECT_EQ(restored.classAbilityLastUse, side.classAbilityLastUse);
	EXPECT_TRUE(serializer.isFullyRead());
}

class AdvancedClassIntegrationTest : public GameStateTest
{
};

TEST_F(AdvancedClassIntegrationTest, ServerSelectionAppliesBonusOnlyOnce)
{
	startTestGame();
	const auto heroes = gameState->getPlayerState(PlayerColor(0))->getHeroes();
	ASSERT_FALSE(heroes.empty());
	auto * hero = gameState->getHero(heroes.front()->id);
	ASSERT_NE(hero, nullptr);

	JsonNode data;
	data["name"].String() = "Integration Test Class";
	data["tier"].Integer() = 1;
	data["requiredLevel"].Integer() = 0;
	data["requiredPoints"].Integer() = 0;
	data["parents"].Vector();
	data["bonuses"]["morale"]["type"].String() = "MORALE";
	data["bonuses"]["morale"]["val"].Integer() = 1;
	LIBRARY->advancedClassHandler->loadObject("advanced-class-test", "integration", data);

	GameHandlerTestServer server(gameState);
	CGameHandler gameHandler(server, gameState);
	const int initialMorale = hero->valOfBonuses(BonusType::MORALE);
	const std::string classId = "advanced-class-test:integration";

	ASSERT_TRUE(gameHandler.selectAdvancedClass(hero->id, classId));
	EXPECT_EQ(hero->advancedClass.classes.front(), classId);
	EXPECT_EQ(hero->valOfBonuses(BonusType::MORALE), initialMorale + 1);

	EXPECT_FALSE(gameHandler.selectAdvancedClass(hero->id, classId));
	EXPECT_EQ(hero->valOfBonuses(BonusType::MORALE), initialMorale + 1);
}
}
