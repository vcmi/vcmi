/*
 * ITranslatorTest.cpp, part of VCMI engine
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
#include "../../lib/texts/ITranslator.h"
#include "../../lib/texts/OverlayTranslator.h"
#include "../../lib/texts/TextLocalizationContainer.h"

namespace test
{

using namespace ::testing;

class ITranslatorTest : public Test {};

// the static store must stay usable through the interface, so that resolution can be
// routed through an ITranslator without changing what any existing caller renders
TEST_F(ITranslatorTest, StaticStoreResolvesThroughInterface)
{
	const ITranslator & translator = *LIBRARY->generaltexth;

	ASSERT_EQ(translator.translate("core.genrltxt", 1), LIBRARY->generaltexth->translate("core.genrltxt", 1));
	ASSERT_EQ(translator.translateString(TextIdentifier("core.genrltxt", 1)), LIBRARY->generaltexth->translateString(TextIdentifier("core.genrltxt", 1)));
}

// a translation mod puts its map string overrides in the static store, while the map carries the
// same keys in its original language - the override has to win, or translated maps read as English
TEST_F(ITranslatorTest, OverriddenMapTextOutranksMapOwnText)
{
	const TextIdentifier translated("map.overlaytranslatortest.translated.name");
	const TextIdentifier untranslated("map.overlaytranslatortest.untranslated.name");

	TextLocalizationContainer mapTexts;
	mapTexts.registerString("map", translated, "original");
	mapTexts.registerString("map", untranslated, "original");

	LIBRARY->generaltexth->registerString("test", translated, "override");

	const OverlayTranslator translator(mapTexts);

	EXPECT_EQ(translator.translateString(translated), "override");
	EXPECT_EQ(translator.translateString(untranslated), "original");
}

}
