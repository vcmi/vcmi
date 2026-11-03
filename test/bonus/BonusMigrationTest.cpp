/*
 * BonusMigrationTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../../lib/bonuses/BonusMigration.h"
#include "../../lib/json/JsonNode.h"

static JsonNode legacyEnchanted(int value)
{
	JsonNode ability;
	ability["type"].String() = "ENCHANTED";
	ability["subtype"].String() = "shield";
	ability["val"].Integer() = value;
	return ability;
}

TEST(BonusMigrationTest, EnchantedKeepsMasteryInValue)
{
	JsonNode migrated;
	ASSERT_TRUE(BonusMigration::migrateBonus(legacyEnchanted(2), migrated));

	EXPECT_EQ(migrated["type"].String(), "COMBAT_EVENT_TRIGGER");
	EXPECT_EQ(migrated["subtype"].String(), "enchanted");
	EXPECT_EQ(migrated["val"].Integer(), 2);
	EXPECT_EQ(migrated["addInfo"]["spell"].String(), "shield");
	EXPECT_FALSE(migrated["addInfo"]["massive"].Bool());
}

TEST(BonusMigrationTest, EnchantedUnpacksMassEffect)
{
	JsonNode migrated;
	ASSERT_TRUE(BonusMigration::migrateBonus(legacyEnchanted(4), migrated));

	EXPECT_EQ(migrated["val"].Integer(), 1);
	EXPECT_TRUE(migrated["addInfo"]["massive"].Bool());
}
