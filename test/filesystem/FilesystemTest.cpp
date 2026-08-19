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

#include "../../lib/filesystem/CFilesystemLoader.h"
#include "../../lib/filesystem/ResourcePath.h"

namespace test
{

class FilesystemTest : public ::testing::Test
{
	boost::filesystem::path directory;

protected:
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

}
