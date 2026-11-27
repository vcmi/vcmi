/*
 * ModSnapshotTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/modding/ModDescription.h"
#include "../../lib/modding/ModManager.h"
#include "../../lib/modding/ModSnapshot.h"

TEST(ModSnapshotTest, keepsModListAfterModManagerChange)
{
	const TModID modID = "vcmi-test";
	ModManager mods(JsonNode(), true);

	const ModSnapshot snapshot = mods.createSnapshot();
	ASSERT_TRUE(snapshot.isModActive(modID));

	mods.tryDisableMod(modID);
	EXPECT_FALSE(mods.createSnapshot().isModActive(modID));
	EXPECT_TRUE(snapshot.isModActive(modID));
	EXPECT_EQ(snapshot.getModDescription(modID).getID(), modID);

	mods.tryEnableMods({modID});
	EXPECT_TRUE(mods.createSnapshot().isModActive(modID));
}
