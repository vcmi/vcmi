/*
 * ModDescriptionTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/modding/ModDescription.h"

TEST(ModDescriptionTest, descriptionPriority)
{
	const auto & mod = LIBRARY->modh->getModInfo("vcmi-test");

	// description/<language>.md overrides legacy description.md
	EXPECT_EQ(mod.getLocalValue("english")["description"].String(), "# English description\n");
	// legacy description.md is used when there is no description/<language>.md
	EXPECT_EQ(mod.getLocalValue("german")["description"].String(), "Legacy German description\n");
	// mod.json is used when there are no description files
	EXPECT_TRUE(mod.getLocalValue("polish")["description"].isNull());
	EXPECT_EQ(mod.getLocalValue("description").String().substr(0, 9), "Test-only");
}
