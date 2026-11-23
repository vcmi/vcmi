/*
 * TreasurePlacerTest.cpp, part of VCMI engine
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
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapObjectConstructors/RandomMapInfo.h"
#include "../../lib/mapObjects/CGPandoraBox.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/rewardable/Info.h"
#include "../../lib/rmg/modificators/TreasurePlacer.h"
#include "../../lib/serializer/JsonDeserializer.h"
#include "../mock/mock_IGameInfoCallback.h"

namespace test
{
using namespace ::testing;

class TreasurePlacerTest : public Test
{
protected:
	static constexpr ui32 STEP = TreasurePlacer::PANDORA_AMOUNT_STEP;

	CMapGenerator::Config config{};

	void SetUp() override
	{
		// Arbitrary test multipliers, deliberately not the config/randomMap.json values
		config.pandoraMultiplierGold = 1000;
		config.pandoraMultiplierExperience = 3000;
	}

	static JsonNode entry(const std::string & text)
	{
		return JsonNode(text.data(), text.size(), "TreasurePlacerTest");
	}

	static const CCreature * creature(const std::string & name)
	{
		auto id = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "creature", name);
		if(!id)
			throw std::runtime_error("unknown creature: " + name);
		return (*LIBRARY->creh)[CreatureID(*id)];
	}

	static std::vector<const CCreature *> castleNatives()
	{
		std::vector<const CCreature *> natives;
		for(const auto * cre : LIBRARY->creh->getObjects())
		{
			if(cre && !cre->special && cre->getFactionID() == FactionID::CASTLE)
				natives.push_back(cre);
		}
		return natives;
	}
};

TEST_F(TreasurePlacerTest, ExperienceOnlyBoxDefaultsToBuiltInPreset)
{
	/*
	 * Asserts: an experience-only box without rmg value or rarity gets the built-in experience
	 * preset: the config multiplier per PANDORA_AMOUNT_STEP of experience, and the preset rarity.
	 * Pass: value is 2 * pandoraMultiplierExperience for 2 steps, rarity is PANDORA_EXPERIENCE_RARITY.
	 * Fail: the entry is rejected, or the value or rarity differs from the built-in box.
	 */

	auto box = TreasurePlacer::readCustomPandoraBox(entry(R"({ "experience" : )" + std::to_string(2 * STEP) + " }"), config);

	ASSERT_TRUE(box.has_value());
	EXPECT_EQ(box->value, 2u * config.pandoraMultiplierExperience);
	EXPECT_EQ(box->rarity, TreasurePlacer::PANDORA_EXPERIENCE_RARITY);
}

TEST_F(TreasurePlacerTest, GoldOnlyBoxDefaultsToBuiltInPreset)
{
	/*
	 * Asserts: a gold-only box without rmg value or rarity gets the built-in gold preset, and
	 * zoneLimit defaults to 0 (unlimited) with no faction restriction.
	 * Pass: value is 2 * pandoraMultiplierGold for 2 steps, rarity is PANDORA_GOLD_RARITY,
	 * zoneLimit is 0 and zoneFactions is empty.
	 * Fail: the entry is rejected, a default differs, or the box gets a zone limit or factions.
	 */

	auto box = TreasurePlacer::readCustomPandoraBox(entry(R"({ "resources" : { "gold" : )" + std::to_string(2 * STEP) + " } }"), config);

	ASSERT_TRUE(box.has_value());
	EXPECT_EQ(box->value, 2u * config.pandoraMultiplierGold);
	EXPECT_EQ(box->rarity, TreasurePlacer::PANDORA_GOLD_RARITY);
	EXPECT_EQ(box->zoneLimit, 0u);
	EXPECT_TRUE(box->zoneFactions.empty());
}

TEST_F(TreasurePlacerTest, ExplicitRmgFieldsWinOverDefaults)
{
	/*
	 * Asserts: rmg value, rarity and zoneLimit set in the entry replace the preset defaults.
	 * Pass: value 12345, rarity 9999 and zoneLimit 1 are read as written.
	 * Fail: the preset value or rarity is used although the entry sets its own.
	 */

	auto box = TreasurePlacer::readCustomPandoraBox(entry(R"({ "experience" : 5000, "rmg" : { "value" : 12345, "rarity" : 9999, "zoneLimit" : 1 } })"), config);

	ASSERT_TRUE(box.has_value());
	EXPECT_EQ(box->value, 12345u);
	EXPECT_EQ(box->rarity, 9999u);
	EXPECT_EQ(box->zoneLimit, 1u);
}

TEST_F(TreasurePlacerTest, BoxWithoutPresetNeedsValueAndRarity)
{
	/*
	 * Asserts: a box that is neither gold-only nor experience-only has no default, so it is
	 * rejected unless both rmg value and rarity are set. Gold plus wood is not gold-only.
	 * Pass: spells without rmg, spells with only a value, and gold plus wood are rejected; spells
	 * with both fields are accepted with those values.
	 * Fail: a box without a preset gets a made-up value or rarity, or a complete entry is rejected.
	 */

	EXPECT_FALSE(TreasurePlacer::readCustomPandoraBox(entry(R"({ "spells" : [ "fly" ] })"), config).has_value());
	EXPECT_FALSE(TreasurePlacer::readCustomPandoraBox(entry(R"({ "spells" : [ "fly" ], "rmg" : { "value" : 8000 } })"), config).has_value());
	EXPECT_FALSE(TreasurePlacer::readCustomPandoraBox(entry(R"({ "resources" : { "gold" : 5000, "wood" : 10 } })"), config).has_value());

	auto box = TreasurePlacer::readCustomPandoraBox(entry(R"({ "spells" : [ "fly" ], "rmg" : { "value" : 8000, "rarity" : 2 } })"), config);
	ASSERT_TRUE(box.has_value());
	EXPECT_EQ(box->value, 8000u);
	EXPECT_EQ(box->rarity, 2u);
}

TEST_F(TreasurePlacerTest, NativeCreaturesBoxDefaultsToBuiltInCreatureBox)
{
	/*
	 * Asserts: an rmg.nativeCreatures entry needs no content, value or rarity: it takes the built-in
	 * creature box rarity, leaves the value to the per-creature built-in formula, and keeps its level;
	 * explicit rmg fields still win, and a missing level means all levels.
	 * Pass: level 1 with PANDORA_CREATURES_RARITY and no value; the second entry has level 0, value
	 * 7000, rarity 9999 and zoneLimit 100.
	 * Fail: the entry is rejected for lacking value or rarity, or a field is read wrong.
	 */

	auto box = TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { "level" : 1 } } })"), config);
	ASSERT_TRUE(box.has_value());
	EXPECT_EQ(box->nativeCreaturesLevel, 1);
	EXPECT_EQ(box->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::ALL);
	EXPECT_EQ(box->rarity, TreasurePlacer::PANDORA_CREATURES_RARITY);
	EXPECT_FALSE(box->value.has_value());

	auto allLevels = TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { }, "value" : 7000, "rarity" : 9999, "zoneLimit" : 100 } })"), config);
	ASSERT_TRUE(allLevels.has_value());
	EXPECT_EQ(allLevels->nativeCreaturesLevel, 0);
	EXPECT_EQ(allLevels->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::ALL);
	EXPECT_EQ(allLevels->value, 7000u);
	EXPECT_EQ(allLevels->rarity, 9999u);
	EXPECT_EQ(allLevels->zoneLimit, 100u);
}

TEST_F(TreasurePlacerTest, NativeCreaturesBoxRejectsOtherContent)
{
	/*
	 * Asserts: rmg.nativeCreatures generates the whole content, so an entry that also has its own
	 * content is rejected instead of silently dropping part of it.
	 * Pass: nativeCreatures plus experience gives nullopt; a plain experience box has no native level.
	 * Fail: the mixed entry is accepted, or a normal box is treated as a native creature box.
	 */

	EXPECT_FALSE(TreasurePlacer::readCustomPandoraBox(entry(R"({ "experience" : 5000, "rmg" : { "nativeCreatures" : { "level" : 1 } } })"), config).has_value());

	auto plain = TreasurePlacer::readCustomPandoraBox(entry(R"({ "experience" : 5000 })"), config);
	ASSERT_TRUE(plain.has_value());
	EXPECT_FALSE(plain->nativeCreaturesLevel.has_value());
	EXPECT_EQ(plain->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::ALL);
}

TEST_F(TreasurePlacerTest, NativeCreaturesKindBasicAndUpgrade)
{
	/*
	 * Asserts: nativeCreatures.kind of basic or upgrade is stored when level is 1 or more.
	 * Pass: level 1 + kind basic is BASIC; level 1 + kind upgrade is UPGRADE.
	 * Fail: kind is left at ALL, or the entry is rejected.
	 */

	auto basic = TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { "level" : 1, "kind" : "basic" } } })"), config);
	ASSERT_TRUE(basic.has_value());
	EXPECT_EQ(basic->nativeCreaturesLevel, 1);
	EXPECT_EQ(basic->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::BASIC);

	auto upgrade = TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { "level" : 1, "kind" : "upgrade" } } })"), config);
	ASSERT_TRUE(upgrade.has_value());
	EXPECT_EQ(upgrade->nativeCreaturesLevel, 1);
	EXPECT_EQ(upgrade->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::UPGRADE);
}

TEST_F(TreasurePlacerTest, NativeCreaturesKindNeedsLevel)
{
	/*
	 * Asserts: kind without a level of 1 or more is ignored; the entry is kept as all natives.
	 * Pass: kind with no level, and kind with level 0, both keep ALL.
	 * Fail: the entry is rejected, or kind is stored without a usable level.
	 */

	auto noLevel = TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { "kind" : "upgrade" } } })"), config);
	ASSERT_TRUE(noLevel.has_value());
	EXPECT_EQ(noLevel->nativeCreaturesLevel, 0);
	EXPECT_EQ(noLevel->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::ALL);

	auto levelZero = TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { "level" : 0, "kind" : "basic" } } })"), config);
	ASSERT_TRUE(levelZero.has_value());
	EXPECT_EQ(levelZero->nativeCreaturesLevel, 0);
	EXPECT_EQ(levelZero->nativeCreaturesKind, TreasurePlacer::NativeCreatureKind::ALL);
}

TEST_F(TreasurePlacerTest, NativeCreaturesUnknownKindIsRejected)
{
	/*
	 * Asserts: an unknown nativeCreatures.kind is an invalid entry, as other invalid pandoraBoxes
	 * entries are.
	 * Pass: kind elite gives nullopt.
	 * Fail: the entry is accepted with ALL, BASIC or UPGRADE.
	 */

	EXPECT_FALSE(TreasurePlacer::readCustomPandoraBox(entry(R"({ "rmg" : { "nativeCreatures" : { "level" : 3, "kind" : "elite" } } })"), config).has_value());
}

TEST_F(TreasurePlacerTest, SelectNativeCreaturesFiltersCastleLevel1)
{
	/*
	 * Asserts: selectNativeCreatures keeps Castle natives of the requested level and kind, in
	 * natives order. Castle level 1 is pikeman (basic) and halberdier (upgrade of pikeman).
	 * Pass: BASIC is {pikeman}, UPGRADE is {halberdier}, ALL is both in that order.
	 * Fail: an upgrade is treated as basic, the pair is empty, or the order is reversed.
	 */

	const auto natives = castleNatives();
	const auto * pikeman = creature("pikeman");
	const auto * halberdier = creature("halberdier");

	TreasurePlacer::CustomPandoraBox box;
	box.nativeCreaturesLevel = 1;

	box.nativeCreaturesKind = TreasurePlacer::NativeCreatureKind::BASIC;
	EXPECT_THAT(TreasurePlacer::selectNativeCreatures(box, natives), ElementsAre(pikeman));

	box.nativeCreaturesKind = TreasurePlacer::NativeCreatureKind::UPGRADE;
	EXPECT_THAT(TreasurePlacer::selectNativeCreatures(box, natives), ElementsAre(halberdier));

	box.nativeCreaturesKind = TreasurePlacer::NativeCreatureKind::ALL;
	EXPECT_THAT(TreasurePlacer::selectNativeCreatures(box, natives), ElementsAre(pikeman, halberdier));
}

TEST_F(TreasurePlacerTest, SelectNativeCreaturesSplitsNativesAndKeepsOrder)
{
	/*
	 * Asserts: over all levels, BASIC and UPGRADE split the natives into two disjoint sets and
	 * keep the order of natives.
	 * Pass: every native is in exactly one of the two results, and each result is a subsequence
	 * of natives.
	 * Fail: a creature is in both, in neither, or the order of natives is not kept.
	 */

	const auto natives = castleNatives();
	ASSERT_FALSE(natives.empty());

	TreasurePlacer::CustomPandoraBox box;
	box.nativeCreaturesLevel = 0;

	box.nativeCreaturesKind = TreasurePlacer::NativeCreatureKind::BASIC;
	const auto basic = TreasurePlacer::selectNativeCreatures(box, natives);
	box.nativeCreaturesKind = TreasurePlacer::NativeCreatureKind::UPGRADE;
	const auto upgrade = TreasurePlacer::selectNativeCreatures(box, natives);

	EXPECT_EQ(basic.size() + upgrade.size(), natives.size());

	size_t basicIndex = 0;
	size_t upgradeIndex = 0;
	for(const auto * native : natives)
	{
		if(basicIndex < basic.size() && basic[basicIndex] == native)
			++basicIndex;
		else if(upgradeIndex < upgrade.size() && upgrade[upgradeIndex] == native)
			++upgradeIndex;
		else
			ADD_FAILURE() << "native is in neither result, or the order of natives is not kept";
	}
	EXPECT_EQ(basicIndex, basic.size());
	EXPECT_EQ(upgradeIndex, upgrade.size());
}

TEST_F(TreasurePlacerTest, SelectNativeCreaturesUpgradeIsMeasuredAgainstNatives)
{
	/*
	 * Asserts: a creature counts as an upgrade only when its base is also in natives, not when
	 * it is an upgrade in the creature table.
	 * Pass: natives of only halberdier + UPGRADE is empty.
	 * Fail: halberdier is selected because pikeman exists in the game data.
	 */

	const auto * halberdier = creature("halberdier");
	TreasurePlacer::CustomPandoraBox box;
	box.nativeCreaturesLevel = 1;
	box.nativeCreaturesKind = TreasurePlacer::NativeCreatureKind::UPGRADE;

	EXPECT_THAT(TreasurePlacer::selectNativeCreatures(box, {halberdier}), IsEmpty());
}

TEST_F(TreasurePlacerTest, ZoneFactionsResolveIncludingNeutral)
{
	/*
	 * Asserts: rmg zoneFactions names resolve to faction ids, "neutral" included (neutral zones
	 * have the neutral faction as their town type).
	 * Pass: the set is exactly { CASTLE, NEUTRAL }.
	 * Fail: a name is not resolved, or neutral is dropped.
	 */

	auto box = TreasurePlacer::readCustomPandoraBox(entry(R"({ "experience" : 5000, "rmg" : { "zoneFactions" : [ "castle", "neutral" ] } })"), config);

	ASSERT_TRUE(box.has_value());
	EXPECT_THAT(box->zoneFactions, UnorderedElementsAre(FactionID::CASTLE, FactionID::NEUTRAL));
}

TEST_F(TreasurePlacerTest, MapOptionsLoadIntoPandoraBox)
{
	/*
	 * Asserts: a pandoraBoxes entry (without rmg), placed under "options", loads into a Pandora box
	 * through the map loader's own CGObjectInstance::serializeJson, with no object-instance
	 * resolver, exactly as TreasurePlacer does when it generates a custom box.
	 * Pass: the box has one first-visit reward granting Town Portal and 1000 experience.
	 * Fail: the short form is not read, the reward is split, or loading needs an instance resolver.
	 */

	JsonNode objectJson;
	objectJson["options"] = entry(R"({ "spells" : [ "townPortal" ], "experience" : 1000 })");
	objectJson.setModScope(ModScope::scopeGame());
	// create() asserts a callback; TreasurePlacer passes the map's, the box only stores it
	NiceMock<IGameInfoCallbackMock> gameInfo;
	std::shared_ptr<CGObjectInstance> object = LIBRARY->objtypeh->getHandlerFor(Obj::PANDORAS_BOX, 0).create(&gameInfo, nullptr);
	auto box = std::dynamic_pointer_cast<CGPandoraBox>(object);
	ASSERT_NE(box, nullptr);

	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	JsonDeserializer handler(nullptr, resolver, objectJson);
	object->serializeJson(handler);

	ASSERT_EQ(box->configuration.info.size(), 1u);
	const auto & info = box->configuration.info.front();
	EXPECT_EQ(info.visitType, Rewardable::EEventType::EVENT_FIRST_VISIT);
	EXPECT_THAT(info.reward.spells, ElementsAre(SpellID(SpellID::decode("townPortal"))));
	EXPECT_EQ(info.reward.heroExperience, 1000);
}

TEST_F(TreasurePlacerTest, ApplyCommonObjectDefaultsFillsValueFromDefaults)
{
	/*
	 * Asserts: a commonObjects entry with only rarity keeps that rarity and takes value from the
	 * object type's rmg defaults.
	 * Pass: value 900 from defaults, probability 500 unchanged, returns true.
	 * Fail: the value stays 0, rarity is overwritten, or the call returns false.
	 */

	ObjectInfo object;
	object.value = 0;
	object.probability = 500;
	RandomMapInfo defaults;
	defaults.value = 900;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 500);
}

TEST_F(TreasurePlacerTest, ApplyCommonObjectDefaultsFillsRarityFromDefaults)
{
	/*
	 * Asserts: a commonObjects entry with only value keeps that value and takes rarity from the
	 * object type's rmg defaults.
	 * Pass: probability 40 from defaults, value 900 unchanged, returns true.
	 * Fail: the rarity stays 0, value is overwritten, or the call returns false.
	 */

	ObjectInfo object;
	object.value = 900;
	object.probability = 0;
	RandomMapInfo defaults;
	defaults.value = 1000;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 40);
}

TEST_F(TreasurePlacerTest, ApplyCommonObjectDefaultsKeepsBothWhenSet)
{
	/*
	 * Asserts: an entry that sets both value and rarity is left unchanged.
	 * Pass: value 900 and probability 50 stay as written, returns true.
	 * Fail: a field is replaced from the defaults.
	 */

	ObjectInfo object;
	object.value = 900;
	object.probability = 50;
	RandomMapInfo defaults;
	defaults.value = 1000;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 50);
}

TEST_F(TreasurePlacerTest, ApplyCommonObjectDefaultsRejectsMissingDefault)
{
	/*
	 * Asserts: an entry without a value is rejected when the object type also has no rmg value.
	 * Pass: returns false when defaults.value is 0 and the entry value is 0.
	 * Fail: the entry is accepted with a made-up value.
	 */

	ObjectInfo object;
	object.value = 0;
	object.probability = 50;
	RandomMapInfo defaults;
	defaults.value = 0;
	defaults.rarity = 40;

	EXPECT_FALSE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
}

}
