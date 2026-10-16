/*
 * SaveGameTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameStateTest.h"

#include "../../lib/campaign/CampaignState.h"
#include "../../lib/gameState/SaveMetadata.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CMemorySerializer.h"
#include "../../lib/serializer/CSaveFile.h"

class SaveGameTest : public GameStateTest
{
protected:
	boost::filesystem::path directory;

	void SetUp() override
	{
		GameStateTest::SetUp();
		directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-savegame-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
	}

	void TearDown() override
	{
		boost::filesystem::remove_all(directory);
		GameStateTest::TearDown();
	}
};

TEST_F(SaveGameTest, savedGameIsRestored)
{
	startTestGame();
	const auto saveFile = directory / "test.vsgm1";

	{
		CMemorySerializer header;
		gameState->saveHeader(header.oser);
		CMemorySerializer game;
		gameState->saveGame(game.oser);

		CSaveFile save;
		save.addChunk(SaveFileFormat::CHUNK_METADATA, SaveMetadata::create(*gameState).toBytes());
		save.addChunk(SaveFileFormat::CHUNK_HEADER, header.extractBuffer());
		save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, game.extractBuffer());
		save.write(saveFile);
	}

	CLoadFile loader(saveFile);

	const JsonNode metadata = SaveMetadata::parse(loader.readChunk(SaveFileFormat::CHUNK_METADATA), "test");
	EXPECT_EQ(metadata["map"]["width"].Integer(), map->width);
	EXPECT_EQ(metadata["game"]["humanPlayers"].Integer(), gameState->getStartInfo()->playerInfos.size());
	EXPECT_NO_THROW(SaveMetadata::verifyMods(metadata));

	CMapHeader loadedHeader;
	StartInfo loadedStartInfo;
	auto headerChunk = loader.openChunk(SaveFileFormat::CHUNK_HEADER, nullptr);
	headerChunk->iser & loadedHeader;
	headerChunk->iser & loadedStartInfo;
	EXPECT_TRUE(headerChunk->isFullyRead());
	EXPECT_EQ(loadedHeader.width, map->width);
	EXPECT_EQ(loadedStartInfo.mapname, gameState->getStartInfo()->mapname);

	auto loadedState = std::make_shared<CGameState>();
	loadedState->preInit(&services);
	auto gameChunk = loader.openChunk(SaveFileFormat::CHUNK_GAMESTATE, loadedState.get());
	loadedState->loadGame(gameChunk->iser);
	EXPECT_TRUE(gameChunk->isFullyRead());
	EXPECT_EQ(loadedState->day, gameState->day);
	EXPECT_EQ(loadedState->getMap().width, map->width);
	EXPECT_EQ(loadedState->players.size(), gameState->players.size());
}

TEST_F(SaveGameTest, invalidMetadataIsRejected)
{
	startTestGame();

	JsonNode metadata = SaveMetadata::create(*gameState);
	metadata["mods"]["testMod"]["name"].String() = "mod without version";
	EXPECT_THROW(SaveMetadata::parse(metadata.toBytes(), "test"), std::runtime_error);

	metadata = SaveMetadata::create(*gameState);
	metadata.Struct().erase("map");
	EXPECT_THROW(SaveMetadata::parse(metadata.toBytes(), "test"), std::runtime_error);
}
