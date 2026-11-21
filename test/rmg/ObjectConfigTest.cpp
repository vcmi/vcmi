/*
 * ObjectConfigTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/rmg/ObjectConfig.h"
#include "../../lib/rmg/ObjectInfo.h"

namespace test
{
using namespace ::testing;

class ObjectConfigTest : public Test
{
protected:
	static CompoundMapObjectID creatureBank(si32 secondary = 0)
	{
		return CompoundMapObjectID(Obj::CREATURE_BANK, secondary);
	}

	static CompoundMapObjectID cartographer(si32 secondary)
	{
		return CompoundMapObjectID(Obj::CARTOGRAPHER, secondary);
	}

	static CompoundMapObjectID scholar()
	{
		return CompoundMapObjectID(Obj::SCHOLAR, 0);
	}

	static CompoundMapObjectID refugeeCamp()
	{
		return CompoundMapObjectID(Obj::REFUGEE_CAMP, 0);
	}

	static CompoundMapObjectID randomDwelling(si32 level)
	{
		return CompoundMapObjectID(Obj::RANDOM_DWELLING_LVL, level);
	}

	ObjectInfo makeCustomObject(CompoundMapObjectID id, ui32 value)
	{
		ObjectInfo info(id);
		info.value = value;
		info.probability = 50;
		return info;
	}

	const ObjectInfo * findConfigured(const ObjectConfig & config, CompoundMapObjectID id)
	{
		for(const auto & object : config.getConfiguredObjects())
		{
			if(object.getCompoundID() == id)
				return &object;
		}
		return nullptr;
	}
};

TEST_F(ObjectConfigTest, EmptyZoneInheritsAllSections)
{
	ObjectConfig base;
	base.setObjectBanned(creatureBank(-1), true);
	base.addBannedCategory(ObjectConfig::EObjectCategory::CREATURE_BANK);
	base.addCustomObject(makeCustomObject(scholar(), 900), *LIBRARY->objtypeh);
	base.addRequiredObject(refugeeCamp(), 2);

	ObjectConfig zone;
	auto merged = ObjectConfig::merge(base, zone);

	EXPECT_TRUE(merged.isObjectBanned(creatureBank(0)));
	ASSERT_TRUE(merged.getObjectBans().count(creatureBank(-1)));
	EXPECT_TRUE(merged.getObjectBans().at(creatureBank(-1)));
	EXPECT_THAT(merged.getBannedObjectCategories(), Contains(ObjectConfig::EObjectCategory::CREATURE_BANK));
	ASSERT_NE(findConfigured(merged, scholar()), nullptr);
	EXPECT_EQ(findConfigured(merged, scholar())->value, 900u);
	ASSERT_TRUE(merged.getRequiredObjects().count(refugeeCamp()));
	EXPECT_EQ(merged.getRequiredObjects().at(refugeeCamp()).first, 2);
}

TEST_F(ObjectConfigTest, ZoneFalseLiftsExactAndTypeLevelBan)
{
	ObjectConfig base;
	base.setObjectBanned(creatureBank(0), true);
	base.setObjectBanned(cartographer(-1), true);

	ObjectConfig zone;
	zone.setObjectBanned(creatureBank(0), false);
	zone.setObjectBanned(cartographer(-1), false);

	auto merged = ObjectConfig::merge(base, zone);

	EXPECT_FALSE(merged.isObjectBanned(creatureBank(0)));
	EXPECT_FALSE(merged.isObjectBanned(cartographer(0)));
	EXPECT_FALSE(merged.isObjectBanned(cartographer(-1)));
}

TEST_F(ObjectConfigTest, ZoneFalseExemptsObjectFromCategoryAndAllBan)
{
	ObjectConfig categoryBase;
	categoryBase.addBannedCategory(ObjectConfig::EObjectCategory::CREATURE_BANK);
	EXPECT_TRUE(categoryBase.isObjectBanned(creatureBank(0)));

	ObjectConfig categoryZone;
	categoryZone.setObjectBanned(creatureBank(0), false);
	auto categoryMerged = ObjectConfig::merge(categoryBase, categoryZone);
	EXPECT_FALSE(categoryMerged.isObjectBanned(creatureBank(0)));

	ObjectConfig allBase;
	allBase.addBannedCategory(ObjectConfig::EObjectCategory::ALL);
	EXPECT_TRUE(allBase.isObjectBanned(scholar()));

	ObjectConfig allZone;
	allZone.setObjectBanned(scholar(), false);
	auto allMerged = ObjectConfig::merge(allBase, allZone);
	EXPECT_FALSE(allMerged.isObjectBanned(scholar()));
}

TEST_F(ObjectConfigTest, ZoneSubtypeBanWinsOverTemplateTypeAllow)
{
	ObjectConfig base;
	base.setObjectBanned(cartographer(-1), false);

	ObjectConfig zone;
	zone.setObjectBanned(cartographer(0), true);

	auto merged = ObjectConfig::merge(base, zone);

	EXPECT_TRUE(merged.isObjectBanned(cartographer(0)));
	EXPECT_FALSE(merged.isObjectBanned(cartographer(1)));
}

TEST_F(ObjectConfigTest, CategoriesAreCombined)
{
	ObjectConfig base;
	base.addBannedCategory(ObjectConfig::EObjectCategory::CREATURE_BANK);

	ObjectConfig zone;
	zone.addBannedCategory(ObjectConfig::EObjectCategory::DWELLING);

	auto merged = ObjectConfig::merge(base, zone);

	EXPECT_THAT(merged.getBannedObjectCategories(), UnorderedElementsAre(
		ObjectConfig::EObjectCategory::CREATURE_BANK,
		ObjectConfig::EObjectCategory::DWELLING));
}

TEST_F(ObjectConfigTest, ZoneCommonObjectReplacesSameIdAndKeepsOthers)
{
	ObjectConfig base;
	base.addCustomObject(makeCustomObject(scholar(), 900), *LIBRARY->objtypeh);
	base.addCustomObject(makeCustomObject(randomDwelling(0), 1000), *LIBRARY->objtypeh);
	base.addCustomObject(makeCustomObject(randomDwelling(6), 7000), *LIBRARY->objtypeh);

	ObjectConfig zone;
	zone.addCustomObject(makeCustomObject(scholar(), 1200), *LIBRARY->objtypeh);
	zone.addCustomObject(makeCustomObject(randomDwelling(6), 15000), *LIBRARY->objtypeh);

	auto merged = ObjectConfig::merge(base, zone);

	ASSERT_NE(findConfigured(merged, scholar()), nullptr);
	EXPECT_EQ(findConfigured(merged, scholar())->value, 1200u);
	ASSERT_NE(findConfigured(merged, randomDwelling(0)), nullptr);
	EXPECT_EQ(findConfigured(merged, randomDwelling(0))->value, 1000u);
	ASSERT_NE(findConfigured(merged, randomDwelling(6)), nullptr);
	EXPECT_EQ(findConfigured(merged, randomDwelling(6))->value, 15000u);
	EXPECT_EQ(merged.getConfiguredObjects().size(), 3u);
}

TEST_F(ObjectConfigTest, ZoneTrueBanDropsInheritedCommonObject)
{
	ObjectConfig base;
	base.addCustomObject(makeCustomObject(scholar(), 900), *LIBRARY->objtypeh);
	base.addCustomObject(makeCustomObject(randomDwelling(0), 1000), *LIBRARY->objtypeh);

	ObjectConfig zone;
	zone.setObjectBanned(scholar(), true);

	auto merged = ObjectConfig::merge(base, zone);

	EXPECT_EQ(findConfigured(merged, scholar()), nullptr);
	ASSERT_NE(findConfigured(merged, randomDwelling(0)), nullptr);
}

TEST_F(ObjectConfigTest, RequiredObjectReplacedAndCountZeroTakesEffect)
{
	ObjectConfig base;
	base.addRequiredObject(refugeeCamp(), 5);
	base.addRequiredObject(scholar(), 1);

	ObjectConfig zone;
	zone.addRequiredObject(refugeeCamp(), 0);
	zone.addRequiredObject(scholar(), 3);

	auto merged = ObjectConfig::merge(base, zone);

	ASSERT_TRUE(merged.getRequiredObjects().count(refugeeCamp()));
	EXPECT_EQ(merged.getRequiredObjects().at(refugeeCamp()).first, 0);
	ASSERT_TRUE(merged.getRequiredObjects().count(scholar()));
	EXPECT_EQ(merged.getRequiredObjects().at(scholar()).first, 3);
}

}
