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

}
