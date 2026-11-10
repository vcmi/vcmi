/*
 * ScenarioMetadataTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/mapping/ScenarioMetadata.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextLocalizationContainer.h"

static const std::string testMod = "vcmi-test";
static const std::string absentMod = "absent-test-mod";

static JsonNode makeMetadataMods(const std::string & testModVersion, const std::vector<std::string> & testSubmods)
{
	JsonNode result;
	result[absentMod]["name"].String() = "Absent";
	result[absentMod]["version"].String() = "1.0";
	result[absentMod]["submods"].Vector();
	result[testMod]["name"].String() = "Test";
	result[testMod]["version"].String() = testModVersion;
	for(const auto & submod : testSubmods)
		result[testMod]["submods"].Vector().emplace_back(submod);
	return result;
}

TEST(ScenarioMetadataTest, saveModsIgnoreVersionsAndSubmods)
{
	const auto result = ScenarioMetadata::verifyMods(makeMetadataMods("2.0", {"absent"}), true);

	EXPECT_EQ(result.at(absentMod), ModVerificationStatus::NOT_INSTALLED);
	EXPECT_FALSE(result.contains(testMod));
	EXPECT_FALSE(result.contains(testMod + ".absent"));
}

TEST(ScenarioMetadataTest, saveWithoutActiveModRequiresDisablingIt)
{
	JsonNode mods;
	mods.Struct();

	const auto result = ScenarioMetadata::verifyMods(mods, true);

	EXPECT_EQ(result.at(testMod), ModVerificationStatus::EXCESSIVE);
}

TEST(ScenarioMetadataTest, mapModsCheckVersionsAndSubmods)
{
	EXPECT_EQ(ScenarioMetadata::verifyMods(makeMetadataMods("2.0", {}), false).at(testMod), ModVerificationStatus::VERSION_MISMATCH);

	const auto result = ScenarioMetadata::verifyMods(makeMetadataMods("1.0", {"absent"}), false);
	EXPECT_EQ(result.at(absentMod), ModVerificationStatus::NOT_INSTALLED);
	EXPECT_EQ(result.at(testMod + ".absent"), ModVerificationStatus::NOT_INSTALLED);
	EXPECT_FALSE(result.contains(testMod));
	EXPECT_EQ(result.size(), 2);
}

static ModVerificationInfo makeMod(const std::string & name, bool impactsGameplay)
{
	ModVerificationInfo result;
	result.name = name;
	result.version = CModVersion::fromString("1.2.3");
	result.impactsGameplay = impactsGameplay;
	return result;
}

TEST(ScenarioMetadataTest, modsAreGroupedByRootMod)
{
	ModCompatibilityInfo mods;
	mods["root"] = makeMod("Root", false);
	mods["root.gameplay"] = makeMod("Gameplay", true);
	mods["root.gameplay.nested"] = makeMod("Nested", true);
	mods["root.visual"] = makeMod("Visual", false);
	mods["cosmetic"] = makeMod("Cosmetic", false);
	mods["core"] = makeMod("Core", true);
	mods["orphan.submod"] = makeMod("Submod", true);

	const JsonNode result = ScenarioMetadata::modsToJson(mods);

	ASSERT_EQ(result.Struct().size(), 2);
	EXPECT_EQ(result["root"]["name"].String(), "Root");
	EXPECT_EQ(result["root"]["version"].String(), "1.2.3");
	ASSERT_EQ(result["root"]["submods"].Vector().size(), 2);
	EXPECT_EQ(result["root"]["submods"][0].String(), "gameplay");
	EXPECT_EQ(result["root"]["submods"][1].String(), "gameplay.nested");

	// mod list of a map may contain a submod without its root mod
	EXPECT_EQ(result["orphan"]["name"].String(), "orphan");
	EXPECT_EQ(result["orphan"]["submods"][0].String(), "submod");
}

TEST(ScenarioMetadataTest, textUsesPreferredLanguage)
{
	const std::string preferred = Languages::getPreferredLanguage();
	const std::string other = preferred == "polish" ? "czech" : "polish";

	JsonNode text;
	MetaString::createFromRawString("raw").jsonSerialize(text["text"]);
	text["translations"][other].String() = "other";
	EXPECT_EQ(ScenarioMetadata::textToString(text, nullptr), "other");

	text["translations"][preferred].String() = "preferred";
	EXPECT_EQ(ScenarioMetadata::textToString(text, nullptr), "preferred");
}

TEST(ScenarioMetadataTest, textWithoutTranslationsUsesMetaString)
{
	JsonNode text;
	MetaString::createFromRawString("raw").jsonSerialize(text["text"]);
	text["translations"].Struct();
	TextLocalizationContainer translator;
	EXPECT_EQ(ScenarioMetadata::textToString(text, &translator), "raw");
}
