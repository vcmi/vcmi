/*
 * SaveFileTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/serializer/CSaveFile.h"

class SaveFileTest : public ::testing::Test
{
protected:
	boost::filesystem::path directory;
	boost::filesystem::path saveFile;

	const std::vector<std::byte> metadata = makeData(100, 1);
	const std::vector<std::byte> header = makeData(5000, 2);
	const std::vector<std::byte> gameState = makeData(200000, 3);

	static std::vector<std::byte> makeData(size_t size, int seed)
	{
		std::vector<std::byte> result(size);
		for(size_t i = 0; i < size; ++i)
			result[i] = static_cast<std::byte>((i * seed + i / 7) % 251);
		return result;
	}

	void SetUp() override
	{
		directory = boost::filesystem::current_path() / boost::filesystem::unique_path("vcmi-savefile-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
		saveFile = directory / "test.vsgm1";

		CSaveFile save;
		save.addChunk(SaveFileFormat::CHUNK_METADATA, metadata);
		save.addChunk(SaveFileFormat::CHUNK_HEADER, header);
		save.addChunk(SaveFileFormat::CHUNK_GAMESTATE, gameState);
		save.write(saveFile);
	}

	void TearDown() override
	{
		boost::filesystem::remove_all(directory);
	}

	std::vector<char> readSaveFile() const
	{
		std::ifstream stream(saveFile.c_str(), std::ios::binary);
		return std::vector<char>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
	}

	void writeSaveFile(const std::vector<char> & content) const
	{
		std::ofstream stream(saveFile.c_str(), std::ios::binary | std::ios::trunc);
		stream.write(content.data(), content.size());
	}
};

TEST_F(SaveFileTest, chunksAreRestored)
{
	EXPECT_EQ(CLoadFile::readVersion(saveFile), ESerializationVersion::CURRENT);

	CLoadFile loader(saveFile);
	// chunks can be read in any order, so that list of saves only needs to read metadata
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_GAMESTATE), gameState);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_METADATA), metadata);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_HEADER), header);
}

TEST_F(SaveFileTest, truncatedFileIsRejected)
{
	auto content = readSaveFile();
	content.resize(content.size() - 100);
	writeSaveFile(content);

	EXPECT_THROW(CLoadFile loader(saveFile), std::runtime_error);
}

TEST_F(SaveFileTest, dataAfterLastChunkIsRejected)
{
	auto content = readSaveFile();
	content.resize(content.size() + 100);
	writeSaveFile(content);

	EXPECT_THROW(CLoadFile loader(saveFile), std::runtime_error);
}

TEST_F(SaveFileTest, corruptedChunkIsRejected)
{
	auto content = readSaveFile();
	content[content.size() - 100] ^= 0x55;
	writeSaveFile(content);

	CLoadFile loader(saveFile);
	EXPECT_EQ(loader.readChunk(SaveFileFormat::CHUNK_METADATA), metadata);
	EXPECT_THROW(loader.readChunk(SaveFileFormat::CHUNK_GAMESTATE), std::runtime_error);
}

TEST_F(SaveFileTest, unsupportedVersionsAreReported)
{
	auto content = readSaveFile();

	content[4] = 1;
	content[5] = 0;
	writeSaveFile(content);
	EXPECT_THROW(CLoadFile::readVersion(saveFile), SaveTooOldException);

	content[6] = 1;
	writeSaveFile(content);
	try
	{
		CLoadFile::readVersion(saveFile);
		FAIL() << "Save from a newer version must be rejected";
	}
	catch(const SaveTooOldException &)
	{
		FAIL() << "Save from a newer version must not be reported as too old";
	}
	catch(const std::runtime_error &)
	{
	}
}
