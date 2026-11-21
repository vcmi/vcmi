/*
 * PandoraBoxConfigTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/CCreatureHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/PandoraBoxConfig.h"
#include "../../lib/serializer/JsonDeserializer.h"
#include "../../lib/serializer/JsonSerializer.h"

namespace test
{
using namespace ::testing;

class PandoraBoxConfigTest : public Test
{
protected:
	CMapGenerator::Config config;

	void SetUp() override
	{
		config.pandoraMultiplierGold = 5000;
		config.pandoraMultiplierExperience = 6000;
		config.pandoraMultiplierSpells = 2500;
		config.pandoraSpellSchool = 15000;
		config.pandoraSpell60 = 30000;
	}

	PandoraBoxConfig loadBox(const char * text)
	{
		JsonNode node(text, std::strlen(text), "PandoraBoxConfigTest");
		PandoraBoxConfig box;
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, node);
		box.serializeJson(handler);
		return box;
	}

	PandoraBoxConfig roundTrip(const PandoraBoxConfig & src)
	{
		JsonNode saved;
		{
			JsonSerializer handler(nullptr, saved);
			const_cast<PandoraBoxConfig &>(src).serializeJson(handler);
		}

		PandoraBoxConfig loaded;
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, saved);
		loaded.serializeJson(handler);
		return loaded;
	}
};

TEST_F(PandoraBoxConfigTest, LoadsRewardFields)
{
	/*
	 * Asserts: reward loads heroExperience, gold, one creature stack and one spell id.
	 * Pass: 5000 experience, 10000 gold, 10 pikemen, magicArrow.
	 * Fail: any of those fields is left at its default.
	 */

	auto box = loadBox(R"({
		"reward" : {
			"heroExperience" : 5000,
			"resources" : { "gold" : 10000 },
			"creatures" : [ { "type" : "pikeman", "amount" : 10 } ],
			"spells" : [ "magicArrow" ]
		}
	})");

	EXPECT_EQ(box.reward.heroExperience, 5000);
	EXPECT_EQ(box.reward.resources[EGameResID::GOLD], 10000);
	ASSERT_EQ(box.reward.creatures.size(), 1u);
	EXPECT_EQ(box.reward.creatures[0].getId(), CreatureID(CreatureID::decode("pikeman")));
	EXPECT_EQ(box.reward.creatures[0].getCount(), 10);
	ASSERT_EQ(box.reward.spells.size(), 1u);
	EXPECT_EQ(box.reward.spells[0], SpellID(SpellID::decode("magicArrow")));
}

TEST_F(PandoraBoxConfigTest, RandomSpellsCountDefaults)
{
	/*
	 * Asserts: randomSpells count is 12 with level, 15 with only school, 60 with neither,
	 * and an explicit count is kept.
	 * Pass: those four counts, plus level 3 and school air.
	 * Fail: count stays 0 or a default overwrites the explicit count.
	 */

	auto withLevel = loadBox(R"({ "randomSpells" : { "level" : 3 } })");
	ASSERT_TRUE(withLevel.randomSpells.has_value());
	EXPECT_EQ(withLevel.randomSpells->count, 12);
	ASSERT_TRUE(withLevel.randomSpells->level.has_value());
	EXPECT_EQ(*withLevel.randomSpells->level, 3);
	EXPECT_FALSE(withLevel.randomSpells->school.has_value());

	auto withSchool = loadBox(R"({ "randomSpells" : { "school" : "air" } })");
	ASSERT_TRUE(withSchool.randomSpells.has_value());
	EXPECT_EQ(withSchool.randomSpells->count, 15);
	EXPECT_FALSE(withSchool.randomSpells->level.has_value());
	ASSERT_TRUE(withSchool.randomSpells->school.has_value());
	EXPECT_EQ(*withSchool.randomSpells->school, SpellSchool::AIR);

	auto withNeither = loadBox(R"({ "randomSpells" : { } })");
	ASSERT_TRUE(withNeither.randomSpells.has_value());
	EXPECT_EQ(withNeither.randomSpells->count, 60);
	EXPECT_FALSE(withNeither.randomSpells->level.has_value());
	EXPECT_FALSE(withNeither.randomSpells->school.has_value());

	auto explicitCount = loadBox(R"({ "randomSpells" : { "level" : 5, "count" : 4 } })");
	ASSERT_TRUE(explicitCount.randomSpells.has_value());
	EXPECT_EQ(explicitCount.randomSpells->count, 4);
	ASSERT_TRUE(explicitCount.randomSpells->level.has_value());
	EXPECT_EQ(*explicitCount.randomSpells->level, 5);
}

TEST_F(PandoraBoxConfigTest, NativeCreaturesAmountOptional)
{
	/*
	 * Asserts: nativeCreatures loads level, and amount is optional.
	 * Pass: level 7 without amount, and level 3 with amount 5.
	 * Fail: amount is forced to a default, or level is not read.
	 */

	auto withoutAmount = loadBox(R"({ "nativeCreatures" : { "level" : 7 } })");
	ASSERT_TRUE(withoutAmount.nativeCreatures.has_value());
	EXPECT_EQ(withoutAmount.nativeCreatures->level, 7);
	EXPECT_FALSE(withoutAmount.nativeCreatures->amount.has_value());

	auto withAmount = loadBox(R"({ "nativeCreatures" : { "level" : 3, "amount" : 5 } })");
	ASSERT_TRUE(withAmount.nativeCreatures.has_value());
	EXPECT_EQ(withAmount.nativeCreatures->level, 3);
	ASSERT_TRUE(withAmount.nativeCreatures->amount.has_value());
	EXPECT_EQ(*withAmount.nativeCreatures->amount, 5u);
}

TEST_F(PandoraBoxConfigTest, RmgFieldsAndDefaultZoneLimit)
{
	/*
	 * Asserts: rmg.value, rmg.rarity and rmg.zoneLimit load, and zoneLimit is 0 when absent.
	 * Pass: explicit 9000 / 7 / 2, and absent zoneLimit is 0.
	 * Fail: missing zoneLimit is treated as unlimited (max) or value/rarity stay unset.
	 */

	auto withRmg = loadBox(R"({ "rmg" : { "value" : 9000, "rarity" : 7, "zoneLimit" : 2 } })");
	ASSERT_TRUE(withRmg.value.has_value());
	EXPECT_EQ(*withRmg.value, 9000u);
	ASSERT_TRUE(withRmg.rarity.has_value());
	EXPECT_EQ(*withRmg.rarity, 7);
	EXPECT_EQ(withRmg.zoneLimit, 2u);

	auto withoutZoneLimit = loadBox(R"({ "rmg" : { "value" : 1000 } })");
	EXPECT_EQ(withoutZoneLimit.zoneLimit, 0u);
	ASSERT_TRUE(withoutZoneLimit.value.has_value());
	EXPECT_EQ(*withoutZoneLimit.value, 1000u);
}

TEST_F(PandoraBoxConfigTest, ZoneFactionsLoadCastleAndNeutral)
{
	/*
	 * Asserts: zoneFactions resolves castle and neutral.
	 * Pass: the set contains FactionID::CASTLE and FactionID::NEUTRAL.
	 * Fail: either identifier is skipped or stored as a different faction.
	 */

	auto box = loadBox(R"({ "zoneFactions" : [ "castle", "neutral" ] })");
	EXPECT_EQ(box.zoneFactions.size(), 2u);
	EXPECT_TRUE(box.zoneFactions.count(FactionID::CASTLE));
	EXPECT_TRUE(box.zoneFactions.count(FactionID::NEUTRAL));
}

TEST_F(PandoraBoxConfigTest, GetValueUsesBuiltInFormulas)
{
	/*
	 * Asserts: default value uses the built-in gold, experience, spell and creature formulas,
	 * adds a passed-in native value, prefers rmg.value, and is 0 for artifacts only.
	 * Pass: 5000 exp → 6000, 10000 gold → 10000, level 3 spells → 10000, school → 15000,
	 * 60 spells → 30000, pikemen stack = aiValue * amount, native 1234 is added, explicit 42 wins,
	 * artifacts only → 0.
	 * Fail: any formula uses the wrong multiplier, count default, or ignores rmg.value.
	 */

	EXPECT_EQ(loadBox(R"({ "reward" : { "heroExperience" : 5000 } })").getValue(config, 0), 6000u);
	EXPECT_EQ(loadBox(R"({ "reward" : { "resources" : { "gold" : 10000 } } })").getValue(config, 0), 10000u);
	EXPECT_EQ(loadBox(R"({ "randomSpells" : { "level" : 3 } })").getValue(config, 0), 10000u);
	EXPECT_EQ(loadBox(R"({ "randomSpells" : { "school" : "air" } })").getValue(config, 0), 15000u);
	EXPECT_EQ(loadBox(R"({ "randomSpells" : { } })").getValue(config, 0), 30000u);

	auto creatures = loadBox(R"({ "reward" : { "creatures" : [ { "type" : "pikeman", "amount" : 10 } ] } })");
	ASSERT_EQ(creatures.reward.creatures.size(), 1u);
	const auto * pikeman = creatures.reward.creatures[0].getCreature();
	ASSERT_NE(pikeman, nullptr);
	EXPECT_EQ(creatures.getValue(config, 0), static_cast<ui32>(pikeman->getAIValue() * 10));

	EXPECT_EQ(loadBox(R"({ "reward" : { "heroExperience" : 5000 } })").getValue(config, 1234), 7234u);
	EXPECT_EQ(loadBox(R"({ "reward" : { "heroExperience" : 5000 }, "rmg" : { "value" : 42 } })").getValue(config, 0), 42u);
	EXPECT_EQ(loadBox(R"({ "reward" : { "artifacts" : [ "ringOfLife" ] } })").getValue(config, 0), 0u);
}

TEST_F(PandoraBoxConfigTest, GetRarityUsesLowestPresentPart)
{
	/*
	 * Asserts: rarity is the lowest built-in rarity among present parts, 2 if none of them is present,
	 * and an explicit rmg.rarity wins.
	 * Pass: experience + gold → 5, spells only → 2, artifacts only → 2, explicit 9 wins over experience.
	 * Fail: rarities are added, experience (20) wins over gold (5), or rmg.rarity is ignored.
	 */

	EXPECT_EQ(loadBox(R"({
		"reward" : { "heroExperience" : 5000, "resources" : { "gold" : 5000 } }
	})").getRarity(), 5);
	EXPECT_EQ(loadBox(R"({ "randomSpells" : { "level" : 5 } })").getRarity(), 2);
	EXPECT_EQ(loadBox(R"({ "reward" : { "artifacts" : [ "ringOfLife" ] } })").getRarity(), 2);
	EXPECT_EQ(loadBox(R"({ "reward" : { "heroExperience" : 5000 }, "rmg" : { "rarity" : 9 } })").getRarity(), 9);
}

TEST_F(PandoraBoxConfigTest, IsAllowedForFaction)
{
	/*
	 * Asserts: an empty zoneFactions set allows every faction, and a non-empty set filters.
	 * Pass: empty allows castle and inferno; [castle, neutral] allows those two and not inferno.
	 * Fail: empty set denies everyone, or the filter is ignored.
	 */

	auto unrestricted = loadBox(R"({ })");
	EXPECT_TRUE(unrestricted.isAllowedForFaction(FactionID::CASTLE));
	EXPECT_TRUE(unrestricted.isAllowedForFaction(FactionID::INFERNO));
	EXPECT_TRUE(unrestricted.isAllowedForFaction(FactionID::NEUTRAL));

	auto filtered = loadBox(R"({ "zoneFactions" : [ "castle", "neutral" ] })");
	EXPECT_TRUE(filtered.isAllowedForFaction(FactionID::CASTLE));
	EXPECT_TRUE(filtered.isAllowedForFaction(FactionID::NEUTRAL));
	EXPECT_FALSE(filtered.isAllowedForFaction(FactionID::INFERNO));
}

TEST_F(PandoraBoxConfigTest, SaveLoadKeepsEveryField)
{
	/*
	 * Asserts: a save → load cycle keeps reward, randomSpells, nativeCreatures, rmg and zoneFactions.
	 * Pass: every field of the loaded box matches the source.
	 * Fail: any optional is dropped, count defaults are re-applied incorrectly, or ids fail to round-trip.
	 */

	auto src = loadBox(R"({
		"reward" : {
			"heroExperience" : 5000,
			"resources" : { "gold" : 10000 },
			"creatures" : [ { "type" : "pikeman", "amount" : 10 } ],
			"spells" : [ "magicArrow" ]
		},
		"randomSpells" : { "level" : 5, "count" : 4, "school" : "air" },
		"nativeCreatures" : { "level" : 7, "amount" : 3 },
		"rmg" : { "value" : 9000, "rarity" : 7, "zoneLimit" : 2 },
		"zoneFactions" : [ "castle", "neutral" ]
	})");

	auto loaded = roundTrip(src);

	EXPECT_EQ(loaded.reward.heroExperience, 5000);
	EXPECT_EQ(loaded.reward.resources[EGameResID::GOLD], 10000);
	ASSERT_EQ(loaded.reward.creatures.size(), 1u);
	EXPECT_EQ(loaded.reward.creatures[0].getId(), src.reward.creatures[0].getId());
	EXPECT_EQ(loaded.reward.creatures[0].getCount(), 10);
	ASSERT_EQ(loaded.reward.spells.size(), 1u);
	EXPECT_EQ(loaded.reward.spells[0], src.reward.spells[0]);

	ASSERT_TRUE(loaded.randomSpells.has_value());
	EXPECT_EQ(loaded.randomSpells->count, 4);
	ASSERT_TRUE(loaded.randomSpells->level.has_value());
	EXPECT_EQ(*loaded.randomSpells->level, 5);
	ASSERT_TRUE(loaded.randomSpells->school.has_value());
	EXPECT_EQ(*loaded.randomSpells->school, SpellSchool::AIR);

	ASSERT_TRUE(loaded.nativeCreatures.has_value());
	EXPECT_EQ(loaded.nativeCreatures->level, 7);
	ASSERT_TRUE(loaded.nativeCreatures->amount.has_value());
	EXPECT_EQ(*loaded.nativeCreatures->amount, 3u);

	ASSERT_TRUE(loaded.value.has_value());
	EXPECT_EQ(*loaded.value, 9000u);
	ASSERT_TRUE(loaded.rarity.has_value());
	EXPECT_EQ(*loaded.rarity, 7);
	EXPECT_EQ(loaded.zoneLimit, 2u);

	EXPECT_EQ(loaded.zoneFactions, src.zoneFactions);
}

}
