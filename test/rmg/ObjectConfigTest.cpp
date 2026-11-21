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
#include "../../lib/serializer/JsonDeserializer.h"
#include "../../lib/serializer/JsonSerializer.h"

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

	static JsonNode pandoraBox(const std::string & text)
	{
		return JsonNode(text.data(), text.size(), "ObjectConfigTest");
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
	/*
	 * Asserts: a zone with no customObjects of its own inherits all four template-level
	 * sections unchanged (bannedObjects, bannedCategories, commonObjects, requiredObjects).
	 * Pass: the merged config still bans creature banks (type-level ban kept as true), keeps the
	 * CREATURE_BANK category, has the scholar at value 900 and requires 2 refugee camps.
	 * Fail: any section is dropped or altered by merging with an empty zone.
	 */

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
	/*
	 * Asserts: a zone ban entry set to false overrides the template's true entry for the same key,
	 * both for an exact subtype (creatureBank:0) and for a whole type (cartographer:-1).
	 * Pass: none of creatureBank:0, cartographer:0 or cartographer:-1 is banned after the merge.
	 * Fail: the template ban survives, i.e. a zone cannot lift a template-level ban.
	 */

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
	/*
	 * Asserts: a per-object false ban in the zone exempts that object from a category ban, both for
	 * a specific category (CREATURE_BANK) and for the ALL category inherited from the template.
	 * Pass: each object is banned by the category alone (sanity check), and is not banned once the
	 * zone marks it false.
	 * Fail: the sanity check fails (category bans not applied), or the category still bans the
	 * object despite the explicit false entry.
	 */

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
	/*
	 * Asserts: a zone's subtype-level ban (cartographer:0 = true) takes precedence over a
	 * template's type-level allow (cartographer:-1 = false), and affects only that subtype.
	 * Pass: cartographer:0 is banned and cartographer:1 is not.
	 * Fail: the type-level false hides the subtype ban, or the subtype ban spreads to other subtypes.
	 */

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
	/*
	 * Asserts: banned categories are combined (set union) rather than replaced by the zone's list.
	 * Pass: the merged config bans exactly CREATURE_BANK (template) and DWELLING (zone).
	 * Fail: either category is missing, or anything else appears in the merged set.
	 */

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
	/*
	 * Asserts: commonObjects merge per object id: a zone entry replaces the template entry with the
	 * same id, and template entries the zone does not mention are kept.
	 * Pass: scholar has the zone value 1200, dwelling level 6 has the zone value 15000, dwelling
	 * level 0 keeps the template value 1000, and there are exactly 3 entries (no duplicates).
	 * Fail: the template value wins for an overridden id, an untouched template entry is lost, or
	 * the same id appears twice.
	 */

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
	/*
	 * Asserts: when the zone bans an object (true), the matching commonObjects entry inherited from
	 * the template is removed, while other inherited entries stay.
	 * Pass: scholar is absent from the configured objects and dwelling level 0 is still present.
	 * Fail: the banned scholar is still configured, or the unrelated dwelling entry is dropped too.
	 */

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
	/*
	 * Asserts: requiredObjects merge per object id with the zone's count replacing the template's,
	 * and a zone count of 0 is a real override (it removes the requirement), not "unset".
	 * Pass: refugee camp is present with count 0 (was 5) and scholar is present with count 3 (was 1).
	 * Fail: the template count survives, or the count-0 entry is dropped so the template's 5 returns.
	 */

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

TEST_F(ObjectConfigTest, PandoraBoxesConcatenateBaseFirst)
{
	/*
	 * Asserts: pandoraBoxes are concatenated by merge, never replaced: template (base) entries
	 * first, then zone (override) entries, even when two entries are identical.
	 * Pass: the merged list is base box 1, base box 2, then the zone box, in that order.
	 * Fail: the zone list replaces the template list, entries are deduplicated, or the order differs.
	 */

	const auto experienceBox = pandoraBox(R"({ "experience" : 5000 })");
	const auto goldBox = pandoraBox(R"({ "resources" : { "gold" : 10000 } })");

	ObjectConfig base;
	base.addPandoraBox(experienceBox);
	base.addPandoraBox(goldBox);

	ObjectConfig zone;
	zone.addPandoraBox(experienceBox);

	auto merged = ObjectConfig::merge(base, zone);

	EXPECT_THAT(merged.getPandoraBoxes(), ElementsAre(experienceBox, goldBox, experienceBox));
}

TEST_F(ObjectConfigTest, PandoraBoxesSurviveZoneBans)
{
	/*
	 * Asserts: no kind of zone ban removes inherited pandoraBoxes: the pandorasBox category, the
	 * "all" category, or an object ban on the Pandora box type.
	 * Pass: after each merge the template box is still the only entry.
	 * Fail: a ban drops the inherited box, as bans do for commonObjects.
	 */

	const auto experienceBox = pandoraBox(R"({ "experience" : 5000 })");

	ObjectConfig base;
	base.addPandoraBox(experienceBox);

	ObjectConfig categoryZone;
	categoryZone.addBannedCategory(ObjectConfig::EObjectCategory::PANDORAS_BOX);
	EXPECT_THAT(ObjectConfig::merge(base, categoryZone).getPandoraBoxes(), ElementsAre(experienceBox));

	ObjectConfig allZone;
	allZone.addBannedCategory(ObjectConfig::EObjectCategory::ALL);
	EXPECT_THAT(ObjectConfig::merge(base, allZone).getPandoraBoxes(), ElementsAre(experienceBox));

	ObjectConfig objectZone;
	objectZone.setObjectBanned(CompoundMapObjectID(Obj::PANDORAS_BOX, -1), true);
	EXPECT_THAT(ObjectConfig::merge(base, objectZone).getPandoraBoxes(), ElementsAre(experienceBox));
}

TEST_F(ObjectConfigTest, PandoraBoxesRoundTripUnchanged)
{
	/*
	 * Asserts: pandoraBoxes entries are stored as written (no parsing at template load) and saved
	 * back unchanged, including the rmg block and the full "rewardable" form; an empty list is not
	 * written at all.
	 * Pass: loading two entries gives those two nodes, saving writes the same array, and saving a
	 * config without boxes writes no "pandoraBoxes" key.
	 * Fail: an entry is altered or dropped on load or save, or an empty array is written.
	 */

	const auto input = pandoraBox(R"({
		"pandoraBoxes" : [
			{ "spells" : [ "townPortal", "fly" ], "rmg" : { "value" : 15000, "rarity" : 2, "zoneFactions" : [ "castle", "neutral" ] } },
			{ "rewardable" : { "info" : [ { "reward" : { "heroExperience" : 1000 } } ] }, "rmg" : { "value" : 1200, "rarity" : 5 } }
		]
	})");

	ObjectConfig loaded;
	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, input);
		loaded.serializeJson(handler, *LIBRARY->objtypeh);
	}
	EXPECT_EQ(loaded.getPandoraBoxes(), input["pandoraBoxes"].Vector());

	JsonNode saved;
	{
		JsonSerializer handler(nullptr, saved);
		loaded.serializeJson(handler, *LIBRARY->objtypeh);
	}
	EXPECT_EQ(saved["pandoraBoxes"], input["pandoraBoxes"]);

	JsonNode savedEmpty;
	{
		JsonSerializer handler(nullptr, savedEmpty);
		ObjectConfig().serializeJson(handler, *LIBRARY->objtypeh);
	}
	EXPECT_FALSE(savedEmpty.Struct().count("pandoraBoxes"));
}

}
