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

#include "../../lib/texts/ITranslator.h"
#include "../../lib/texts/MetaString.h"

namespace test
{

using namespace ::testing;

/// Resolves any identifier into itself, so that expectations can be written without loading game texts
class EchoTranslator final : public ITranslator
{
	mutable std::string storage;
public:
	const std::string & translateString(const TextIdentifier & identifier) const override
	{
		storage = identifier.get() == "core.genrltxt.141" ? " and " : identifier.get();
		return storage;
	}
};

class MetaStringTest : public Test
{
protected:
	EchoTranslator translator;
};

TEST_F(MetaStringTest, BuildListSeparatesTextID)
{
	MetaString list;
	list.appendTextID("a");
	list.appendTextID("b");
	list.appendTextID("c");

	ASSERT_EQ(list.buildList(&translator), "a, b and c");
}

TEST_F(MetaStringTest, BuildListMixesEntryKinds)
{
	MetaString list;
	list.appendRawString("a");
	list.appendTextID("b");

	ASSERT_EQ(list.buildList(&translator), "a and b");
}

TEST_F(MetaStringTest, BuildListIgnoresReplacements)
{
	MetaString list;
	list.appendRawString("%s");
	list.replaceTextID("a");
	list.appendRawString("b");

	ASSERT_EQ(list.buildList(&translator), "a and b");
}

TEST_F(MetaStringTest, BuildListSingleEntry)
{
	MetaString list;
	list.appendTextID("a");

	ASSERT_EQ(list.buildList(&translator), "a");
}

}
