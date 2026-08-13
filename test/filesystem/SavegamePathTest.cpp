/*
 * SavegamePathTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/StartInfo.h"
#include "../../lib/callback/Calendar.h"
#include "../../lib/filesystem/SavegamePath.h"
#include "../../lib/mapping/CMapHeader.h"

#include <vstd/DateUtils.h>

namespace test
{

namespace
{

StartInfo makeStartInfo()
{
	StartInfo startInfo;
	startInfo.saveDirectory = "A map 2026-08-16 2";
	return startInfo;
}

}

TEST(SavegamePathTest, UsesStoredGameDirectory)
{
	auto startInfo = makeStartInfo();
	CMapHeader mapHeader;

	EXPECT_EQ(SavegamePath::getGameDirectoryName(startInfo, mapHeader), "A map 2026-08-16 2/");
	EXPECT_EQ(
		SavegamePath::getPath(startInfo, mapHeader, "Quicksave"),
		"Saves/A map 2026-08-16 2/Quicksave");
}

TEST(SavegamePathTest, BuildsDirectoryNameForOldSave)
{
	auto startInfo = makeStartInfo();
	CMapHeader mapHeader;
	startInfo.saveDirectory.clear();
	startInfo.startTime = 123456789;
	mapHeader.name = MetaString::createFromRawString("A map: with/illegal? name");
	const std::string startDate = vstd::getFormattedDateTime(startInfo.startTime, "%Y-%m-%d");

	EXPECT_EQ(SavegamePath::getGameDirectoryName(startInfo, mapHeader), "A map_ with_illegal_ name " + startDate + " 1/");
}

TEST(SavegamePathTest, NamesAutosaveAfterGameDate)
{
	const auto startInfo = makeStartInfo();
	CMapHeader mapHeader;
	const auto & gameSettings = *LIBRARY->engineSettings();

	EXPECT_EQ(
		SavegamePath::getAutosavePath(startInfo, mapHeader, Calendar(gameSettings, 1)),
		"Saves/A map 2026-08-16 2/Autosave-111");
}

TEST(SavegamePathTest, RecognizesReservedAutosaveNames)
{
	EXPECT_TRUE(SavegamePath::isAutosaveName("Autosave-111"));
	EXPECT_TRUE(SavegamePath::isAutosaveName("Saves/Game/autosave-123.vsgm1"));
	EXPECT_FALSE(SavegamePath::isAutosaveName("111"));
	EXPECT_FALSE(SavegamePath::isAutosaveName("Autosave-old"));
	EXPECT_FALSE(SavegamePath::isAutosaveName("Autosave-111-copy"));
}

}
