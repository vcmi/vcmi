/*
 * TextOperationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/texts/TextOperations.h"

namespace test
{

using namespace ::testing;

class TextOperationsTest : public Test {};

TEST_F(TextOperationsTest, Test)
{
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("", ""), 0);
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("", "Fortune"), 0);
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("forgot", "forget"), 1);
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("besi", "wbęśiem"), 2);
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("DisPELL", "dispell"), 0);
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("irebal", "Fireball"), 0);
	ASSERT_EQ(TextOperations::textSearchSimilarityScore("irebaw", "Fireball"), 1);
}

// conversion descriptors are cached per thread, so a conversion must not be affected by
// whatever the same thread converted before it - including a conversion that failed
TEST_F(TextOperationsTest, ToUnicodeReusesDescriptor)
{
	const std::string cyrillicCP1251 = "\xcf\xf0\xe8\xe2\xe5\xf2";
	const std::string cyrillicUtf8 = "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82";
	const std::string invalidUtf8 = "\xff\xfe";

	ASSERT_EQ(TextOperations::toUnicode(cyrillicCP1251, "CP1251"), cyrillicUtf8);
	ASSERT_EQ(TextOperations::toUnicode(cyrillicCP1251, "CP1251"), cyrillicUtf8);

	// interleaved encodings must not share a descriptor
	ASSERT_EQ(TextOperations::toUnicode("\xc4", "CP1252"), "\xc3\x84");
	ASSERT_EQ(TextOperations::toUnicode(cyrillicCP1251, "CP1251"), cyrillicUtf8);

	ASSERT_TRUE(TextOperations::toUnicode(invalidUtf8, "UTF-8").empty());
	ASSERT_EQ(TextOperations::toUnicode(cyrillicCP1251, "CP1251"), cyrillicUtf8);

	ASSERT_TRUE(TextOperations::toUnicode(cyrillicCP1251, "NoSuchEncoding").empty());
	ASSERT_EQ(TextOperations::toUnicode(cyrillicCP1251, "CP1251"), cyrillicUtf8);

	// descriptors are per thread, so parallel conversions may not corrupt each other
	std::vector<std::thread> threads;
	std::atomic<int> failures = 0;
	for(int i = 0; i < 8; ++i)
	{
        threads.emplace_back([&failures, &cyrillicCP1251, &cyrillicUtf8]
		{
			for(int j = 0; j < 1000; ++j)
				if(TextOperations::toUnicode(cyrillicCP1251, "CP1251") != cyrillicUtf8)
					++failures;
		});
	}
	for(auto & thread : threads)
		thread.join();

	ASSERT_EQ(failures, 0);
}

}
