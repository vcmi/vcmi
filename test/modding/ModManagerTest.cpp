/*
 * ModManagerTest.cpp, part of VCMI engine
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
#include "../../lib/modding/ModManager.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/modding/ModSnapshot.h"

static JsonNode makeRequiredMods(const std::map<TModID, std::vector<std::string>> & mods)
{
	JsonNode result;
	result.Struct();
	for(const auto & [rootID, submods] : mods)
	{
		result[rootID]["name"].String() = rootID;
		result[rootID]["version"].String() = "1.0";
		result[rootID]["submods"].Vector();
		for(const auto & submod : submods)
			result[rootID]["submods"].Vector().emplace_back(submod);
	}
	return result;
}

TEST(ModManagerTest, exactModListDisablesOtherGameplayMods)
{
	ModManager mods(JsonNode(), true);
	const JsonNode required = makeRequiredMods({{"test-gameplay", {}}});

	const ModSnapshot snapshot = mods.createSnapshot(required, true);

	EXPECT_TRUE(snapshot.isModActive(ModScope::scopeBuiltin()));
	EXPECT_TRUE(snapshot.isModActive("test-gameplay"));
	EXPECT_TRUE(snapshot.isModActive("test-gameplay.visual"));
	EXPECT_FALSE(snapshot.isModActive("test-gameplay.extra"));
	EXPECT_FALSE(snapshot.isModActive("test-gameplay.optional"));
	EXPECT_FALSE(snapshot.isModActive("vcmi-test"));
	EXPECT_TRUE(ScenarioMetadata::verifyMods(required, true, snapshot).empty());

	EXPECT_TRUE(mods.createSnapshot().isModActive("vcmi-test"));
	EXPECT_FALSE(mods.createSnapshot().isModActive("test-gameplay"));
}

TEST(ModManagerTest, minimalModListKeepsActiveModsAndEnablesListedSubmods)
{
	ModManager mods(JsonNode(), true);
	const JsonNode required = makeRequiredMods({{"test-gameplay", {"optional"}}});

	const ModSnapshot exactSnapshot = mods.createSnapshot(required, true);
	EXPECT_TRUE(exactSnapshot.isModActive("test-gameplay.optional"));
	EXPECT_FALSE(exactSnapshot.isModActive("test-gameplay.extra"));
	EXPECT_TRUE(ScenarioMetadata::verifyMods(required, true, exactSnapshot).empty());

	const ModSnapshot snapshot = mods.createSnapshot(required, false);
	EXPECT_TRUE(snapshot.isModActive("test-gameplay.optional"));
	EXPECT_TRUE(snapshot.isModActive("test-gameplay.extra"));
	EXPECT_TRUE(snapshot.isModActive("test-gameplay.visual"));
	EXPECT_TRUE(snapshot.isModActive("vcmi-test"));
	EXPECT_TRUE(ScenarioMetadata::verifyMods(required, false, snapshot).empty());
}

TEST(ModManagerTest, conflictingRequiredModsAreReported)
{
	ModManager mods(JsonNode(), true);
	const JsonNode required = makeRequiredMods({{"test-gameplay", {}}, {"test-conflict", {}}});

	const ModSnapshot snapshot = mods.createSnapshot(required, true);

	EXPECT_NE(snapshot.isModActive("test-gameplay"), snapshot.isModActive("test-conflict"));
	EXPECT_EQ(ScenarioMetadata::verifyMods(required, true, snapshot).size(), 1);
}

TEST(ModManagerTest, requiredModsReplaceConflictingAndDependentActiveMods)
{
	ModManager mods(JsonNode(), true);
	mods.tryEnableMods({"test-gameplay-addon"});
	ASSERT_TRUE(mods.createSnapshot().isModActive("test-gameplay"));
	ASSERT_TRUE(mods.createSnapshot().isModActive("test-gameplay-addon"));

	const ModSnapshot conflictSnapshot = mods.createSnapshot(makeRequiredMods({{"test-conflict", {}}}), false);
	EXPECT_TRUE(conflictSnapshot.isModActive("test-conflict"));
	EXPECT_FALSE(conflictSnapshot.isModActive("test-gameplay"));
	EXPECT_FALSE(conflictSnapshot.isModActive("test-gameplay-addon"));

	const ModSnapshot emptySnapshot = mods.createSnapshot(makeRequiredMods({}), true);
	EXPECT_FALSE(emptySnapshot.isModActive("test-gameplay"));
	EXPECT_FALSE(emptySnapshot.isModActive("test-gameplay-addon"));

	const ModSnapshot listedSnapshot = mods.createSnapshot(makeRequiredMods({{"test-gameplay", {"extra"}}}), true);
	EXPECT_TRUE(listedSnapshot.isModActive("test-gameplay-addon"));

	mods.tryDisableMod("test-gameplay-addon");
	mods.tryDisableMod("test-gameplay");
	EXPECT_FALSE(mods.createSnapshot().isModActive("test-gameplay"));
}
