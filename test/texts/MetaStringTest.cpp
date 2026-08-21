/*
 * MetaStringTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"

namespace test
{

using namespace ::testing;

class MetaStringTest : public Test {};

// emptiness is decided from the stored operations alone - resolving to find out would need a
// translator that most callers do not have, and would fail outright on map text
TEST_F(MetaStringTest, EmptyIgnoresContentThatRendersToNothing)
{
	EXPECT_TRUE(MetaString().empty());
	EXPECT_TRUE(MetaString::createFromRawString("").empty());
	EXPECT_FALSE(MetaString::createFromRawString("text").empty());
}

TEST_F(MetaStringTest, Append)
{
	MetaString suffix;
	suffix.appendRawString(" world");
	suffix.appendNumber(42);

	MetaString text;
	text.appendRawString("hello");
	text.append(suffix);

	ASSERT_EQ(text.toString(LIBRARY->staticTexts()), "hello world42");
}

TEST_F(MetaStringTest, AppendedReplacementActsOnCombinedText)
{
	MetaString suffix;
	suffix.replaceRawString("there");

	MetaString text;
	text.appendRawString("hello %s");
	text.append(suffix);

	ASSERT_EQ(text.toString(LIBRARY->staticTexts()), "hello there");
}

TEST_F(MetaStringTest, ReplaceToken)
{
	MetaString text;
	text.appendRawString("%POINTS of %REMAINING, %POINTS");
	text.replaceTokenNumber("%POINTS", 100);
	text.replaceTokenTextID("%REMAINING", TextIdentifier("core.genrltxt", 1).get());

	// only the first occurrence of a token is replaced, matching the '%s' ops
	ASSERT_EQ(text.toString(LIBRARY->staticTexts()), "100 of " + LIBRARY->generaltexth->translate("core.genrltxt", 1) + ", %POINTS");
}

TEST_F(MetaStringTest, ReplaceTokenSurvivesSerialization)
{
	MetaString text;
	text.appendRawString("%TOWN at level %LEVEL");
	text.replaceTokenTextID("%TOWN", TextIdentifier("core.genrltxt", 1).get());
	text.replaceTokenNumber("%LEVEL", 7);

	JsonNode json;
	text.jsonSerialize(json);

	MetaString restored;
	restored.jsonDeserialize(json);

	ASSERT_EQ(restored, text);
	ASSERT_EQ(restored.toString(LIBRARY->staticTexts()), text.toString(LIBRARY->staticTexts()));
}

}
