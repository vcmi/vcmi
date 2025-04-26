/*
 * CTextOperationsTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../lib/texts/TextOperations.h"

namespace test
{
TEST(BreakTextTest, SimpleSplit)
{
	const auto expectedRes = std::vector<std::string>{
		"Hello  Might", "and Magic;",
		"Hello", "", "Might and", "Magic"};

	EXPECT_EQ(TextOperations::breakText("Hello  Might and Magic; Hello\n\n  Might and Magic", 12,
		[](const std::string & str)->size_t {return str.length();}), expectedRes);
};

TEST(BreakTextTest, ColoredSplit)
{
	const auto expectedRes = std::vector<std::string>{
		"{Hello}", "{Might}", "", "{and}", "{Magic};",
		"{#A9A9A9|Hello}", "{#A9A9A9|Might}", "{#A9A9A9|and}", "{#A9A9A9|Magic};",
		"{green|Hello}", "{green|Might}", "{green|and}", "{green|Magic}"};

	EXPECT_EQ(TextOperations::breakText(
		"{Hello  Might\n\n and Magic}; {#A9A9A9|Hello  Might\n  and Magic}; {green|Hello  Might  and Magic}", 0,
		[](const std::string & str)->size_t {return str.length();}), expectedRes);
};
}
