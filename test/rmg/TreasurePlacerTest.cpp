/*
 * TreasurePlacerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/mapObjectConstructors/RandomMapInfo.h"
#include "../../lib/rmg/ObjectInfo.h"
#include "../../lib/rmg/modificators/TreasurePlacer.h"

namespace test
{
using namespace ::testing;

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsFillsValueFromDefaults)
{
	/*
	 * Asserts: a commonObjects entry with only rarity keeps that rarity and takes value from the
	 * object type's rmg defaults.
	 * Pass: value 900 from defaults, probability 500 unchanged, returns true.
	 * Fail: the value stays 0, rarity is overwritten, or the call returns false.
	 */

	ObjectInfo object;
	object.value = 0;
	object.probability = 500;
	RandomMapInfo defaults;
	defaults.value = 900;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 500);
}

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsFillsRarityFromDefaults)
{
	/*
	 * Asserts: a commonObjects entry with only value keeps that value and takes rarity from the
	 * object type's rmg defaults.
	 * Pass: probability 40 from defaults, value 900 unchanged, returns true.
	 * Fail: the rarity stays 0, value is overwritten, or the call returns false.
	 */

	ObjectInfo object;
	object.value = 900;
	object.probability = 0;
	RandomMapInfo defaults;
	defaults.value = 1000;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 40);
}

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsKeepsBothWhenSet)
{
	/*
	 * Asserts: an entry that sets both value and rarity is left unchanged.
	 * Pass: value 900 and probability 50 stay as written, returns true.
	 * Fail: a field is replaced from the defaults.
	 */

	ObjectInfo object;
	object.value = 900;
	object.probability = 50;
	RandomMapInfo defaults;
	defaults.value = 1000;
	defaults.rarity = 40;

	EXPECT_TRUE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
	EXPECT_EQ(object.value, 900u);
	EXPECT_EQ(object.probability, 50);
}

TEST(TreasurePlacerTest, ApplyCommonObjectDefaultsRejectsMissingDefault)
{
	/*
	 * Asserts: an entry without a value is rejected when the object type also has no rmg value.
	 * Pass: returns false when defaults.value is 0 and the entry value is 0.
	 * Fail: the entry is accepted with a made-up value.
	 */

	ObjectInfo object;
	object.value = 0;
	object.probability = 50;
	RandomMapInfo defaults;
	defaults.value = 0;
	defaults.rarity = 40;

	EXPECT_FALSE(TreasurePlacer::applyCommonObjectDefaults(object, defaults));
}

}
