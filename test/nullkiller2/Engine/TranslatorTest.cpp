/*
 * TranslatorTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "AI/Nullkiller2/Engine/Translator.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/texts/CGeneralTextHandler.h"

// the AI is not linked against the client, so it resolves log text through its own
// translator - which must render identifiers exactly like the static store does
TEST(Nullkiller2_Engine_Translator, ResolvesStaticStoreIdentifiers)
{
	const NK2AI::Translator translator;
	const ITranslator & asInterface = translator;

	EXPECT_EQ(asInterface.translate("core.genrltxt", 1), LIBRARY->generaltexth->translate("core.genrltxt", 1));
	EXPECT_EQ(
		asInterface.translateString(TextIdentifier("core.arraytxt", 23)),
		LIBRARY->generaltexth->translateString(TextIdentifier("core.arraytxt", 23)));
}
