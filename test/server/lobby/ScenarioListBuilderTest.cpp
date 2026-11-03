/*
 * ScenarioListBuilderTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../../server/lobby/ScenarioListBuilder.h"

namespace test
{

namespace
{

std::vector<std::string> names(const std::vector<ResourcePath> & files)
{
	std::vector<std::string> result;
	for(const auto & file : files)
		result.push_back(file.getOriginalName());
	std::ranges::sort(result);
	return result;
}

const std::vector<ResourcePath> saves = {
	ResourcePath("Saves/Quicksave", EResType::SAVEGAME),
	ResourcePath("Saves/Game 1/Save", EResType::SAVEGAME),
	ResourcePath("Saves/Game 1/Autosave/Day 1", EResType::SAVEGAME),
	ResourcePath("Saves/Game 10/Save", EResType::SAVEGAME),
};

}

TEST(ScenarioListBuilderTest, RootListsFilesAndTopLevelFolders)
{
	const auto result = ScenarioListBuilder::splitFolder(saves, "");

	EXPECT_EQ(result.folder, "");
	EXPECT_EQ(names(result.files), std::vector<std::string>{"Saves/Quicksave"});
	ASSERT_EQ(result.subfolders.size(), 2);
	EXPECT_EQ(names(result.subfolders.at("Game 1")), (std::vector<std::string>{"Saves/Game 1/Autosave/Day 1", "Saves/Game 1/Save"}));
	EXPECT_EQ(names(result.subfolders.at("Game 10")), std::vector<std::string>{"Saves/Game 10/Save"});
}

TEST(ScenarioListBuilderTest, FolderIsMatchedIgnoringLetterCase)
{
	const auto result = ScenarioListBuilder::splitFolder(saves, "GAME 1/");

	EXPECT_EQ(result.folder, "Game 1/");
	EXPECT_EQ(names(result.files), std::vector<std::string>{"Saves/Game 1/Save"});
	ASSERT_EQ(result.subfolders.size(), 1);
	EXPECT_EQ(names(result.subfolders.at("Autosave")), std::vector<std::string>{"Saves/Game 1/Autosave/Day 1"});
}

TEST(ScenarioListBuilderTest, MissingFolderIsEmpty)
{
	const auto result = ScenarioListBuilder::splitFolder(saves, "Game 2/");

	EXPECT_EQ(result.folder, "Game 2/");
	EXPECT_TRUE(result.files.empty());
	EXPECT_TRUE(result.subfolders.empty());
}

}
