/*
 * HeroSecondarySkillsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/serializer/JsonSerializer.h"

class HeroSecondarySkillsTest : public GameStateTest
{
protected:
	void SetUp() override
	{
		GameStateTest::SetUp();
		learningSkill().offerCooldown = 1; // library is shared between tests
	}

	void TearDown() override
	{
		learningSkill().offerCooldown = 0;
		GameStateTest::TearDown();
	}

	static CSkill & learningSkill()
	{
		return *LIBRARY->skillh->objects.at(SecondarySkill::LEARNING);
	}

	/// First hero of first player with given skills at basic level
	CGHeroInstance * prepareHeroWithSkills(const std::vector<SecondarySkill> & skills)
	{
		const auto heroes = gameState->getPlayerState(PlayerColor(0))->getHeroes();
		if(heroes.empty())
			return nullptr;

		auto * hero = gameState->getHero(heroes.front()->id);
		hero->secSkills.clear();
		for(const auto & skill : skills)
			hero->setSecSkillLevel(skill, MasteryLevel::BASIC, ChangeValueMode::ABSOLUTE);
		return hero;
	}

	/// Rolls level-up offers like CGameHandler::levelUpHero
	std::vector<SecondarySkill> rollLevelUpOffers(GameRandomizer & randomizer, const CGHeroInstance * hero)
	{
		randomizer.rollPrimarySkillForLevelup(hero); // creates per-hero skill seed
		return randomizer.rollSecondarySkills(hero);
	}
};

TEST_F(HeroSecondarySkillsTest, skillWithOfferCooldownIsWithheldOnFollowingLevelUp)
{
	startTestGame();

	const SecondarySkill learning(SecondarySkill::LEARNING);
	const SecondarySkill wisdom(SecondarySkill::WISDOM);

	auto * hero = prepareHeroWithSkills({learning, wisdom});
	ASSERT_NE(hero, nullptr);
	hero->secSkillsGainedAtLevel[learning] = hero->level; // gained on the previous level-up

	GameRandomizer randomizer(*gameState);
	for(int i = 0; i < 30; ++i)
	{
		const auto offered = rollLevelUpOffers(randomizer, hero);
		EXPECT_FALSE(vstd::contains(offered, learning)) << "roll " << i;
		EXPECT_TRUE(vstd::contains(offered, wisdom)) << "roll " << i;
	}
}

TEST_F(HeroSecondarySkillsTest, skillWithOfferCooldownIsOfferedAgainOnceCooldownPassed)
{
	startTestGame();

	const SecondarySkill learning(SecondarySkill::LEARNING);
	const SecondarySkill wisdom(SecondarySkill::WISDOM);

	auto * hero = prepareHeroWithSkills({learning, wisdom});
	ASSERT_NE(hero, nullptr);
	hero->secSkillsGainedAtLevel[learning] = hero->level - 1; // gained two level-ups ago, cooldown is 1

	GameRandomizer randomizer(*gameState);
	int learningOffers = 0;
	for(int i = 0; i < 40; ++i)
		learningOffers += vstd::contains(rollLevelUpOffers(randomizer, hero), learning) ? 1 : 0;

	EXPECT_GT(learningOffers, 0);
}

TEST_F(HeroSecondarySkillsTest, longerOfferCooldownWithholdsSkillForMoreLevelUps)
{
	startTestGame();

	const SecondarySkill learning(SecondarySkill::LEARNING);
	const SecondarySkill wisdom(SecondarySkill::WISDOM);

	auto * hero = prepareHeroWithSkills({learning, wisdom});
	ASSERT_NE(hero, nullptr);
	learningSkill().offerCooldown = 2;
	hero->secSkillsGainedAtLevel[learning] = hero->level - 1; // gained two level-ups ago, cooldown is 2

	GameRandomizer randomizer(*gameState);
	for(int i = 0; i < 30; ++i)
		EXPECT_FALSE(vstd::contains(rollLevelUpOffers(randomizer, hero), learning)) << "roll " << i;
}

TEST_F(HeroSecondarySkillsTest, gainingSkillWithOfferCooldownRecordsHeroLevel)
{
	startTestGame();

	const SecondarySkill learning(SecondarySkill::LEARNING);
	const SecondarySkill wisdom(SecondarySkill::WISDOM);

	auto * hero = prepareHeroWithSkills({});
	ASSERT_NE(hero, nullptr);
	hero->level = 5;

	hero->setSecSkillLevel(learning, MasteryLevel::BASIC, ChangeValueMode::ABSOLUTE);
	hero->setSecSkillLevel(wisdom, MasteryLevel::BASIC, ChangeValueMode::ABSOLUTE);
	ASSERT_EQ(hero->secSkillsGainedAtLevel.count(learning), 1u);
	EXPECT_EQ(hero->secSkillsGainedAtLevel.at(learning), 5u);
	EXPECT_EQ(hero->secSkillsGainedAtLevel.count(wisdom), 0u) << "no cooldown, not tracked";

	hero->level = 7;
	hero->setSecSkillLevel(learning, MasteryLevel::ADVANCED, ChangeValueMode::ABSOLUTE);
	EXPECT_EQ(hero->secSkillsGainedAtLevel.at(learning), 7u) << "upgrade refreshes record";

	hero->level = 9;
	hero->setSecSkillLevel(learning, MasteryLevel::ADVANCED, ChangeValueMode::ABSOLUTE);
	EXPECT_EQ(hero->secSkillsGainedAtLevel.at(learning), 7u) << "same mastery is not a gain";
}

TEST_F(HeroSecondarySkillsTest, skillWithOfferCooldownIsOfferedIfNothingElseCanBeUpgraded)
{
	startTestGame();

	const SecondarySkill learning(SecondarySkill::LEARNING);

	auto * hero = prepareHeroWithSkills({learning});
	ASSERT_NE(hero, nullptr);
	hero->secSkillsGainedAtLevel[learning] = hero->level;

	GameRandomizer randomizer(*gameState);
	for(int i = 0; i < 10; ++i)
		EXPECT_TRUE(vstd::contains(rollLevelUpOffers(randomizer, hero), learning)) << "roll " << i;
}

TEST_F(HeroSecondarySkillsTest, experienceToGainLevelsSpansWholeLevels)
{
	startTestGame();

	auto * hero = prepareHeroWithSkills({});
	ASSERT_NE(hero, nullptr);

	const auto & heroes = *LIBRARY->heroh;
	EXPECT_EQ(hero->experienceToGainLevels(1), heroes.reqExp(hero->level + 1) - heroes.reqExp(hero->level));
	EXPECT_EQ(hero->experienceToGainLevels(2), heroes.reqExp(hero->level + 2) - heroes.reqExp(hero->level));
	EXPECT_EQ(hero->experienceToGainLevels(0), 0u);

	hero->level = heroes.maxSupportedLevel();
	EXPECT_EQ(hero->experienceToGainLevels(1), 0u);
}

TEST_F(HeroSecondarySkillsTest, startingSkillsRecordStartingLevel)
{
	startTestGame();

	const SecondarySkill learning(SecondarySkill::LEARNING);

	CGHeroInstance hero(gameState.get());
	hero.secSkills = {{learning, MasteryLevel::EXPERT}}; // expert, so auto level-up can not upgrade it
	hero.exp = LIBRARY->heroh->reqExp(5);

	GameRandomizer randomizer(*gameState);
	hero.initHero(randomizer, HeroTypeID(0), true);

	EXPECT_EQ(hero.level, 5u);
	ASSERT_EQ(hero.secSkillsGainedAtLevel.count(learning), 1u);
	EXPECT_EQ(hero.secSkillsGainedAtLevel.at(learning), 1u);
}

// Regression test for GitHub issue #7598.
// A freshly created hero carries a {NONE, -1} "use hero type default skills" marker in
// secSkills. Setting skills explicitly must drop that marker, otherwise the hero
// serializes as "has default skills" and every explicitly set skill is silently lost
// (see CGHeroInstance::serializeJsonOptions). Battle Only Mode hits this whenever all
// 8 skill slots are filled, because the marker was previously only removed as a side
// effect of assigning an empty slot.
TEST_F(HeroSecondarySkillsTest, settingSkillsClearsDefaultSkillsMarker)
{
	startTestGame();

	CGHeroInstance hero(gameState.get());

	ASSERT_EQ(hero.secSkills.size(), 1u) << "fresh hero should carry the default-skills marker";
	ASSERT_EQ(hero.secSkills[0].first, SecondarySkill(SecondarySkill::NONE));

	// Mimics BattleOnlyModeTab::startBattle filling all 8 skill slots.
	const std::vector<SecondarySkill> chosen = {
		SecondarySkill(SecondarySkill::EARTH_MAGIC),
		SecondarySkill(SecondarySkill::AIR_MAGIC),
		SecondarySkill(SecondarySkill::PATHFINDING),
		SecondarySkill(SecondarySkill::ARCHERY),
		SecondarySkill(SecondarySkill::LOGISTICS),
		SecondarySkill(SecondarySkill::SCOUTING),
		SecondarySkill(SecondarySkill::DIPLOMACY),
		SecondarySkill(SecondarySkill::NAVIGATION),
	};

	for (const auto & skill : chosen)
		hero.setSecSkillLevel(skill, MasteryLevel::EXPERT, ChangeValueMode::ABSOLUTE);

	for (const auto & entry : hero.secSkills)
		EXPECT_NE(entry.first, SecondarySkill(SecondarySkill::NONE))
			<< "default-skills marker must not survive alongside explicitly set skills";

	EXPECT_EQ(hero.secSkills.size(), chosen.size());
}

// End-to-end counterpart of the above: the skills must actually survive serialization
// into the map file. Before the fix, a hero with all 8 slots filled wrote no
// "secondarySkills" field at all, so reloading the map silently restored the hero
// type's default skills - which is what made Expert magic schools vanish in battle.
TEST_F(HeroSecondarySkillsTest, allEightSkillsAreSerializedIntoMap)
{
	startTestGame();

	// serializeJsonOptions is protected; expose it exactly as the map saver reaches it.
	struct HeroProbe : public CGHeroInstance
	{
		using CGHeroInstance::CGHeroInstance;
		using CGHeroInstance::serializeJsonOptions;
	};

	HeroProbe hero(gameState.get());
	hero.setHeroType(HeroTypeID(0));
	hero.setOwner(PlayerColor(0));

	const std::vector<SecondarySkill> chosen = {
		SecondarySkill(SecondarySkill::EARTH_MAGIC),
		SecondarySkill(SecondarySkill::AIR_MAGIC),
		SecondarySkill(SecondarySkill::PATHFINDING),
		SecondarySkill(SecondarySkill::ARCHERY),
		SecondarySkill(SecondarySkill::LOGISTICS),
		SecondarySkill(SecondarySkill::SCOUTING),
		SecondarySkill(SecondarySkill::DIPLOMACY),
		SecondarySkill(SecondarySkill::NAVIGATION),
	};

	for (const auto & skill : chosen)
		hero.setSecSkillLevel(skill, MasteryLevel::EXPERT, ChangeValueMode::ABSOLUTE);

	JsonNode written;
	JsonSerializer handler(nullptr, written);
	hero.serializeJsonOptions(handler);

	const JsonNode & skills = written["secondarySkills"];
	ASSERT_EQ(skills.getType(), JsonNode::JsonType::DATA_VECTOR)
		<< "secondarySkills field must be present in the saved map";
	EXPECT_EQ(skills.Vector().size(), chosen.size());

	for (const auto & entry : skills.Vector())
		EXPECT_EQ(entry["level"].String(), "expert");
}
