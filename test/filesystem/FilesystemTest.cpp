/*
 * FilesystemTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/json/JsonNode.h"

namespace test
{

class FilesystemTest : public ::testing::Test
{
	boost::filesystem::path directory;

protected:
	std::vector<std::string> addedInitialLoaders;
	std::vector<std::string> addedBundledLoaders;

	const boost::filesystem::path & getDirectory() const
	{
		return directory;
	}

	void SetUp() override
	{
		directory = boost::filesystem::current_path()
			/ boost::filesystem::unique_path("vcmi-filesystem-%%%%-%%%%-%%%%-%%%%");
		ASSERT_TRUE(boost::filesystem::create_directory(directory));
	}

	void TearDown() override
	{
		for (const auto & loaderId : addedInitialLoaders)
			CResourceHandler::removeFilesystem("initial", loaderId);
		for (const auto & loaderId : addedBundledLoaders)
			CResourceHandler::removeFilesystem("bundled", loaderId);
		boost::filesystem::remove_all(directory);
	}
};

TEST_F(FilesystemTest, RemovesResourceAndUpdatesIndex)
{
	CFilesystemLoader loader("Test/", getDirectory());
	const ResourcePath resource("Test/example.txt");

	ASSERT_TRUE(loader.createResource("Test/example.txt"));
	ASSERT_TRUE(loader.existsResource(resource));
	ASSERT_TRUE(boost::filesystem::exists(getDirectory() / "example.txt"));

	EXPECT_TRUE(loader.removeResource(resource));
	EXPECT_FALSE(loader.existsResource(resource));
	EXPECT_FALSE(boost::filesystem::exists(getDirectory() / "example.txt"));
}

TEST_F(FilesystemTest, DoesNotCreateResourceOutsideOfDirectory)
{
	const auto loaderDirectory = getDirectory() / "loader";
	ASSERT_TRUE(boost::filesystem::create_directory(loaderDirectory));
	CFilesystemLoader loader("Test/", loaderDirectory);

	EXPECT_FALSE(loader.createResource("Test/../example.txt"));
	EXPECT_FALSE(loader.createResource("Test/folder/../../example.txt"));
	EXPECT_FALSE(loader.createResource("Test/" + (getDirectory() / "example.txt").string()));
	EXPECT_FALSE(boost::filesystem::exists(getDirectory() / "example.txt"));

	EXPECT_TRUE(loader.createResource("Test/folder/example.txt"));
	EXPECT_TRUE(boost::filesystem::exists(loaderDirectory / "folder" / "example.txt"));
}

TEST_F(FilesystemTest, FindsFileWithRepeatedExtensionByItsName)
{
	CFilesystemLoader loader("Test/", getDirectory());
	ASSERT_TRUE(loader.createResource("Test/example.h3m.h3m"));

	const ResourcePath storedName(ResourcePath("Test/example.h3m.h3m").getOriginalName(), EResType::MAP);
	EXPECT_TRUE(loader.existsResource(storedName));
}

TEST_F(FilesystemTest, DirectoryResourceGenerationDeduplicatesPathsPerMountAndKeepsLastOverride)
{
	const std::string resourceDirectory = "filesystem-test-resource";
	const auto pathA = getDirectory() / "A";
	const auto pathB = getDirectory() / "B";
	const auto pathC = getDirectory() / "C";
	ASSERT_TRUE(boost::filesystem::create_directories(pathA / resourceDirectory));
	ASSERT_TRUE(boost::filesystem::create_directories(pathB / resourceDirectory));
	ASSERT_TRUE(boost::filesystem::create_directories(pathC / resourceDirectory));
	ASSERT_TRUE(boost::filesystem::create_directories(pathA / resourceDirectory / "child"));
	ASSERT_TRUE(boost::filesystem::create_directories(pathB / resourceDirectory / "child"));
	{
		std::ofstream file((pathA / resourceDirectory / "example.txt").c_str());
		ASSERT_TRUE(file.is_open());
		file << "A";
	}
	{
		std::ofstream file((pathB / resourceDirectory / "example.txt").c_str());
		ASSERT_TRUE(file.is_open());
		file << "B";
	}
	{
		std::ofstream file((pathA / resourceDirectory / "child" / "nested.txt").c_str());
		ASSERT_TRUE(file.is_open());
		file << "A nested";
	}
	{
		std::ofstream file((pathB / resourceDirectory / "child" / "nested.txt").c_str());
		ASSERT_TRUE(file.is_open());
		file << "B nested";
	}

	{
		std::ofstream file((pathC / resourceDirectory / "example.txt").c_str());
		ASSERT_TRUE(file.is_open());
		file << "C";
	}

	CResourceHandler::initialize();

	addedInitialLoaders = {
		"filesystem-test-directory-A1",
		"filesystem-test-directory-B",
		"filesystem-test-directory-A2"};
	CResourceHandler::addFilesystem("initial", addedInitialLoaders[0], std::make_unique<CFilesystemLoader>("", pathA, 0, true));
	CResourceHandler::addFilesystem("initial", addedInitialLoaders[1], std::make_unique<CFilesystemLoader>("", pathB, 0, true));
	CResourceHandler::addFilesystem("initial", addedInitialLoaders[2], std::make_unique<CFilesystemLoader>("", pathA, 0, true));
	addedBundledLoaders = {"filesystem-test-bundled-C1", "filesystem-test-bundled-C2"};
	CResourceHandler::addFilesystem("bundled", addedBundledLoaders[0], std::make_unique<CFilesystemLoader>("", pathC, 0, true));
	CResourceHandler::addFilesystem("bundled", addedBundledLoaders[1], std::make_unique<CFilesystemLoader>("", pathC, 0, true));

	JsonMap testDirectoryEntry;
	testDirectoryEntry["path"] = JsonNode(resourceDirectory);
	testDirectoryEntry["type"] = JsonNode("dir");
	testDirectoryEntry["depth"] = JsonNode(0);
	JsonMap otherDirectoryEntry;
	otherDirectoryEntry["path"] = JsonNode(resourceDirectory);
	otherDirectoryEntry["type"] = JsonNode("dir");
	otherDirectoryEntry["depth"] = JsonNode(1);
	JsonMap bundledDirectoryEntry;
	bundledDirectoryEntry["path"] = JsonNode(resourceDirectory);
	bundledDirectoryEntry["type"] = JsonNode("dir");
	bundledDirectoryEntry["depth"] = JsonNode(0);
	bundledDirectoryEntry["bundledOnly"] = JsonNode(true);
	JsonNode testMountConfig;
	testMountConfig.Vector().push_back(JsonNode(testDirectoryEntry));
	JsonNode otherMountConfig;
	otherMountConfig.Vector().push_back(JsonNode(otherDirectoryEntry));
	JsonNode bundledMountConfig;
	bundledMountConfig.Vector().push_back(JsonNode(bundledDirectoryEntry));
	JsonMap filesystemConfig;
	filesystemConfig["Test/"] = testMountConfig;
	filesystemConfig["Other/"] = otherMountConfig;
	filesystemConfig["Bundled/"] = bundledMountConfig;

	auto generated = CResourceHandler::createFileSystem("", JsonNode(filesystemConfig));
	ASSERT_NE(generated, nullptr);

	const ResourcePath testResource("Test/example.txt");
	const auto testSources = generated->getResourcesWithName(testResource);
	ASSERT_EQ(testSources.size(), 3);
	auto testContents = generated->load(testResource)->readAll();
	EXPECT_EQ(std::string(reinterpret_cast<const char *>(testContents.first.get()), testContents.second), "A");

	const ResourcePath otherResource("Other/example.txt");
	const auto otherSources = generated->getResourcesWithName(otherResource);
	ASSERT_EQ(otherSources.size(), 3);
	auto otherContents = generated->load(otherResource)->readAll();
	EXPECT_EQ(std::string(reinterpret_cast<const char *>(otherContents.first.get()), otherContents.second), "A");
	EXPECT_FALSE(generated->existsResource(ResourcePath("Test/child/nested.txt")));
	EXPECT_TRUE(generated->existsResource(ResourcePath("Other/child/nested.txt")));

	const ResourcePath bundledResource("Bundled/example.txt");
	const auto bundledSources = generated->getResourcesWithName(bundledResource);
	ASSERT_EQ(bundledSources.size(), 1);
	auto bundledContents = generated->load(bundledResource)->readAll();
	EXPECT_EQ(std::string(reinterpret_cast<const char *>(bundledContents.first.get()), bundledContents.second), "C");

	generated.reset();
}

TEST_F(FilesystemTest, DirectoryResourceGenerationKeepsSymlinkPathsDistinct)
{
	const auto source = getDirectory() / "source";
	const auto alias = getDirectory() / "alias";
	ASSERT_TRUE(boost::filesystem::create_directories(source / "filesystem-test-resource"));
	{
		std::ofstream file((source / "filesystem-test-resource" / "example.txt").c_str());
		ASSERT_TRUE(file.is_open());
		file << "content";
	}

	boost::system::error_code error;
	boost::filesystem::create_directory_symlink(source, alias, error);
	if(error)
		GTEST_SKIP() << "Directory symlinks are unavailable: " << error.message();

	CResourceHandler::initialize();
	addedInitialLoaders = {"filesystem-test-symlink-source", "filesystem-test-symlink-alias", "filesystem-test-symlink-duplicate"};
	CResourceHandler::addFilesystem("initial", addedInitialLoaders[0], std::make_unique<CFilesystemLoader>("", source, 0, true));
	CResourceHandler::addFilesystem("initial", addedInitialLoaders[1], std::make_unique<CFilesystemLoader>("", alias, 0, true));
	CResourceHandler::addFilesystem("initial", addedInitialLoaders[2], std::make_unique<CFilesystemLoader>("", source, 0, true));

	JsonNode filesystemConfig;
	auto & entry = filesystemConfig["Test/"].Vector().emplace_back();
	entry["path"].String() = "filesystem-test-resource";
	entry["type"].String() = "dir";
	auto generated = CResourceHandler::createFileSystem("", filesystemConfig);
	ASSERT_NE(generated, nullptr);
	const auto sources = generated->getResourcesWithName(ResourcePath("Test/example.txt"));
	EXPECT_EQ(sources.size(), 2);
}

TEST(ResourcePathTest, KeepsExtensionOfDifferentType)
{
	EXPECT_EQ(ResourcePath("Fonts/CALLI10R.FNT.txt").getName(), "FONTS/CALLI10R.FNT");
}

}
