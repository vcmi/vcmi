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
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/modding/ModScope.h"
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

	static SpellID spellId(const std::string & name)
	{
		auto id = LIBRARY->identifiers()->getIdentifier("core", "spell", name);
		if(!id)
			throw std::runtime_error("unknown spell: " + name);
		return SpellID(*id);
	}

	static ObjectConfig loadConfig(const std::string & text)
	{
		JsonNode json(text.data(), text.size(), "ObjectConfigTest");
		// Identifier requests need a mod scope; mod loading sets it, a test string does not
		json.setModScope(ModScope::scopeBuiltin());
		ObjectConfig config;
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, json);
		config.serializeJson(handler, *LIBRARY->objtypeh);
		return config;
	}

	static JsonNode saveConfig(ObjectConfig & config)
	{
		JsonNode saved;
		JsonSerializer handler(nullptr, saved);
		config.serializeJson(handler, *LIBRARY->objtypeh);
		return saved;
	}

	const ObjectConfig::SpellScrollInfo * findScroll(const ObjectConfig & config, SpellID spell)
	{
		for(const auto & scroll : config.getSpellScrolls())
		{
			if(scroll.spell == spell)
				return &scroll;
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

TEST_F(ObjectConfigTest, SpellScrollsLoadAllFieldsAndSpellOnly)
{
	/*
	 * Asserts: spellScrolls load rmg.value, rmg.rarity and rmg.zoneLimit when set, and leave value
	 * and rarity unset with zoneLimit 0 when only spell is given.
	 * Pass: townPortal has value 6000, rarity 9999, zoneLimit 1; magicArrow has no value or rarity
	 * and zoneLimit 0.
	 * Fail: a field is dropped, a default is filled in at load, or the spell-only entry is skipped.
	 */

	auto loaded = loadConfig(R"({
		"spellScrolls" : [
			{ "spell" : "townPortal", "rmg" : { "value" : 6000, "rarity" : 9999, "zoneLimit" : 1 } },
			{ "spell" : "magicArrow" }
		]
	})");

	const auto townPortal = findScroll(loaded, spellId("townPortal"));
	ASSERT_NE(townPortal, nullptr);
	ASSERT_TRUE(townPortal->value.has_value());
	EXPECT_EQ(*townPortal->value, 6000u);
	ASSERT_TRUE(townPortal->rarity.has_value());
	EXPECT_EQ(*townPortal->rarity, 9999);
	EXPECT_EQ(townPortal->zoneLimit, 1u);

	const auto magicArrow = findScroll(loaded, spellId("magicArrow"));
	ASSERT_NE(magicArrow, nullptr);
	EXPECT_FALSE(magicArrow->value.has_value());
	EXPECT_FALSE(magicArrow->rarity.has_value());
	EXPECT_EQ(magicArrow->zoneLimit, 0u);
}

TEST_F(ObjectConfigTest, SpellScrollsDropEntryWithoutSpell)
{
	/*
	 * Asserts: a spellScrolls entry without a spell string is dropped, while a valid neighbour is kept.
	 * Pass: only the fly entry is loaded.
	 * Fail: the incomplete entry is stored, or the valid entry is dropped with it.
	 */

	auto loaded = loadConfig(R"({
		"spellScrolls" : [
			{ "rmg" : { "value" : 6000 } },
			{ "spell" : "fly" }
		]
	})");

	EXPECT_EQ(loaded.getSpellScrolls().size(), 1u);
	ASSERT_NE(findScroll(loaded, spellId("fly")), nullptr);
}

TEST_F(ObjectConfigTest, RequiredScrollsStayOutOfRequiredObjects)
{
	/*
	 * Asserts: requiredObjects.spellScroll number and { count, guard } forms go to getRequiredScrolls,
	 * while a normal required object stays in getRequiredObjects; clearRequiredObjects keeps the scrolls.
	 * Pass: dimensionDoor count 2 with no guard, townPortal count 1 guard 20000, refugeeCamp count 5
	 * in required objects, and after clearRequiredObjects the scrolls remain and refugeeCamp is gone.
	 * Fail: a scroll lands in getRequiredObjects, a field is dropped, or clearRequiredObjects clears scrolls.
	 */

	auto loaded = loadConfig(R"({
		"requiredObjects" : {
			"refugeeCamp" : 5,
			"spellScroll" : {
				"dimensionDoor" : 2,
				"townPortal" : { "count" : 1, "guard" : 20000 }
			}
		}
	})");

	const CompoundMapObjectID refugeeCampType(Obj::REFUGEE_CAMP, -1);
	ASSERT_TRUE(loaded.getRequiredObjects().count(refugeeCampType));
	EXPECT_EQ(loaded.getRequiredObjects().at(refugeeCampType).first, 5);
	EXPECT_EQ(loaded.getRequiredObjects().size(), 1u);

	const auto dimensionDoor = spellId("dimensionDoor");
	const auto townPortal = spellId("townPortal");
	ASSERT_TRUE(loaded.getRequiredScrolls().count(dimensionDoor));
	EXPECT_EQ(loaded.getRequiredScrolls().at(dimensionDoor).first, 2);
	EXPECT_FALSE(loaded.getRequiredScrolls().at(dimensionDoor).second.has_value());
	ASSERT_TRUE(loaded.getRequiredScrolls().count(townPortal));
	EXPECT_EQ(loaded.getRequiredScrolls().at(townPortal).first, 1);
	ASSERT_TRUE(loaded.getRequiredScrolls().at(townPortal).second.has_value());
	EXPECT_EQ(*loaded.getRequiredScrolls().at(townPortal).second, 20000u);

	loaded.clearRequiredObjects();
	EXPECT_TRUE(loaded.getRequiredObjects().empty());
	ASSERT_TRUE(loaded.getRequiredScrolls().count(dimensionDoor));
	EXPECT_EQ(loaded.getRequiredScrolls().at(dimensionDoor).first, 2);
	ASSERT_TRUE(loaded.getRequiredScrolls().count(townPortal));
}

TEST_F(ObjectConfigTest, SpellScrollsAndRequiredScrollsRoundTrip)
{
	/*
	 * Asserts: save then load keeps every spellScrolls and required-scroll field, and empty lists
	 * are not written.
	 * Pass: townPortal keeps value, rarity and zoneLimit; fly keeps only spell; dimensionDoor keeps
	 * count; townPortal required keeps count and guard; an empty config writes neither key.
	 * Fail: a field is dropped or filled in, or an empty array/object is written.
	 */

	auto loaded = loadConfig(R"({
		"spellScrolls" : [
			{ "spell" : "townPortal", "rmg" : { "value" : 6000, "rarity" : 9999, "zoneLimit" : 1 } },
			{ "spell" : "fly" }
		],
		"requiredObjects" : {
			"spellScroll" : {
				"dimensionDoor" : 2,
				"townPortal" : { "count" : 1, "guard" : 20000 }
			}
		}
	})");

	JsonNode saved = saveConfig(loaded);
	saved.setModScope(ModScope::scopeBuiltin());
	ObjectConfig reloaded;
	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, saved);
		reloaded.serializeJson(handler, *LIBRARY->objtypeh);
	}

	ASSERT_EQ(reloaded.getSpellScrolls().size(), 2u);
	const auto townPortalScroll = findScroll(reloaded, spellId("townPortal"));
	ASSERT_NE(townPortalScroll, nullptr);
	ASSERT_TRUE(townPortalScroll->value.has_value());
	EXPECT_EQ(*townPortalScroll->value, 6000u);
	ASSERT_TRUE(townPortalScroll->rarity.has_value());
	EXPECT_EQ(*townPortalScroll->rarity, 9999);
	EXPECT_EQ(townPortalScroll->zoneLimit, 1u);
	const auto flyScroll = findScroll(reloaded, spellId("fly"));
	ASSERT_NE(flyScroll, nullptr);
	EXPECT_FALSE(flyScroll->value.has_value());
	EXPECT_FALSE(flyScroll->rarity.has_value());
	EXPECT_EQ(flyScroll->zoneLimit, 0u);

	const auto dimensionDoor = spellId("dimensionDoor");
	const auto townPortal = spellId("townPortal");
	ASSERT_TRUE(reloaded.getRequiredScrolls().count(dimensionDoor));
	EXPECT_EQ(reloaded.getRequiredScrolls().at(dimensionDoor).first, 2);
	EXPECT_FALSE(reloaded.getRequiredScrolls().at(dimensionDoor).second.has_value());
	ASSERT_TRUE(reloaded.getRequiredScrolls().count(townPortal));
	EXPECT_EQ(reloaded.getRequiredScrolls().at(townPortal).first, 1);
	ASSERT_TRUE(reloaded.getRequiredScrolls().at(townPortal).second.has_value());
	EXPECT_EQ(*reloaded.getRequiredScrolls().at(townPortal).second, 20000u);

	JsonNode savedEmpty;
	{
		JsonSerializer handler(nullptr, savedEmpty);
		ObjectConfig().serializeJson(handler, *LIBRARY->objtypeh);
	}
	EXPECT_FALSE(savedEmpty.Struct().count("spellScrolls"));
	EXPECT_FALSE(savedEmpty.Struct().count("requiredObjects"));
}

TEST_F(ObjectConfigTest, SpellScrollsMergeReplacesSameSpell)
{
	/*
	 * Asserts: a zone spellScrolls entry for townPortal replaces the template one; a template fly
	 * entry is kept and comes before the zone entries.
	 * Pass: merged list is fly (template rarity 30) then townPortal (zone rarity 9999).
	 * Fail: the template townPortal survives, fly is lost, or fly comes after the zone entry.
	 */

	ObjectConfig::SpellScrollInfo templateTownPortal;
	templateTownPortal.spell = spellId("townPortal");
	templateTownPortal.rarity = 10;
	ObjectConfig::SpellScrollInfo templateFly;
	templateFly.spell = spellId("fly");
	templateFly.rarity = 30;
	ObjectConfig::SpellScrollInfo zoneTownPortal;
	zoneTownPortal.spell = spellId("townPortal");
	zoneTownPortal.rarity = 9999;

	ObjectConfig base;
	base.addSpellScroll(templateTownPortal);
	base.addSpellScroll(templateFly);

	ObjectConfig zone;
	zone.addSpellScroll(zoneTownPortal);

	auto merged = ObjectConfig::merge(base, zone);
	ASSERT_EQ(merged.getSpellScrolls().size(), 2u);
	EXPECT_EQ(merged.getSpellScrolls()[0].spell, spellId("fly"));
	ASSERT_TRUE(merged.getSpellScrolls()[0].rarity.has_value());
	EXPECT_EQ(*merged.getSpellScrolls()[0].rarity, 30);
	EXPECT_EQ(merged.getSpellScrolls()[1].spell, spellId("townPortal"));
	ASSERT_TRUE(merged.getSpellScrolls()[1].rarity.has_value());
	EXPECT_EQ(*merged.getSpellScrolls()[1].rarity, 9999);
}

TEST_F(ObjectConfigTest, SpellScrollsSurviveZoneBans)
{
	/*
	 * Asserts: a zone spellScroll category ban, a type-level spellScroll object ban, and an ALL
	 * category ban keep inherited spellScrolls and required scrolls.
	 * Pass: each merge still has the template townPortal pool entry and dimensionDoor required scroll.
	 * Fail: a ban drops either, as bans do for commonObjects.
	 */

	ObjectConfig::SpellScrollInfo townPortal;
	townPortal.spell = spellId("townPortal");
	townPortal.rarity = 9999;

	ObjectConfig base;
	base.addSpellScroll(townPortal);
	base.addRequiredScroll(spellId("dimensionDoor"), 2);

	ObjectConfig categoryZone;
	categoryZone.addBannedCategory(ObjectConfig::EObjectCategory::SPELL_SCROLL);
	auto categoryMerged = ObjectConfig::merge(base, categoryZone);
	ASSERT_NE(findScroll(categoryMerged, spellId("townPortal")), nullptr);
	ASSERT_TRUE(categoryMerged.getRequiredScrolls().count(spellId("dimensionDoor")));

	ObjectConfig objectZone;
	objectZone.setObjectBanned(CompoundMapObjectID(Obj::SPELL_SCROLL, -1), true);
	auto objectMerged = ObjectConfig::merge(base, objectZone);
	ASSERT_NE(findScroll(objectMerged, spellId("townPortal")), nullptr);
	ASSERT_TRUE(objectMerged.getRequiredScrolls().count(spellId("dimensionDoor")));

	ObjectConfig allZone;
	allZone.addBannedCategory(ObjectConfig::EObjectCategory::ALL);
	auto allMerged = ObjectConfig::merge(base, allZone);
	ASSERT_NE(findScroll(allMerged, spellId("townPortal")), nullptr);
	ASSERT_TRUE(allMerged.getRequiredScrolls().count(spellId("dimensionDoor")));
}

TEST_F(ObjectConfigTest, RequiredScrollReplacedAndCountZeroTakesEffect)
{
	/*
	 * Asserts: required scrolls merge per spell with the zone's count replacing the template's, and
	 * a zone count of 0 is a real override, not "unset".
	 * Pass: townPortal is present with count 0 (was 2) and dimensionDoor is present with count 3 (was 1).
	 * Fail: the template count survives, or the count-0 entry is dropped so the template's 2 returns.
	 */

	const auto townPortal = spellId("townPortal");
	const auto dimensionDoor = spellId("dimensionDoor");

	ObjectConfig base;
	base.addRequiredScroll(townPortal, 2);
	base.addRequiredScroll(dimensionDoor, 1);

	ObjectConfig zone;
	zone.addRequiredScroll(townPortal, 0);
	zone.addRequiredScroll(dimensionDoor, 3);

	auto merged = ObjectConfig::merge(base, zone);
	ASSERT_TRUE(merged.getRequiredScrolls().count(townPortal));
	EXPECT_EQ(merged.getRequiredScrolls().at(townPortal).first, 0);
	ASSERT_TRUE(merged.getRequiredScrolls().count(dimensionDoor));
	EXPECT_EQ(merged.getRequiredScrolls().at(dimensionDoor).first, 3);
}

TEST_F(ObjectConfigTest, DefaultScrollValueBySpellLevel)
{
	/*
	 * Asserts: getDefaultScrollValue indexes scrollValues by the spell's level (1-based), and returns
	 * 0 when that level has no entry.
	 * Pass: magicArrow (level 1) is 500 and townPortal (level 4) is 4000 for five values; townPortal
	 * is 0 when only three values are given.
	 * Fail: the index is 0-based, the wrong spell level is used, or a missing level throws.
	 */

	const std::vector<int> fiveValues{500, 2000, 3000, 4000, 5000};
	EXPECT_EQ(ObjectConfig::getDefaultScrollValue(spellId("magicArrow"), fiveValues), 500u);
	EXPECT_EQ(ObjectConfig::getDefaultScrollValue(spellId("townPortal"), fiveValues), 4000u);

	const std::vector<int> threeValues{500, 2000, 3000};
	EXPECT_EQ(ObjectConfig::getDefaultScrollValue(spellId("townPortal"), threeValues), 0u);
}

TEST_F(ObjectConfigTest, CommonObjectLoadsRarityOnlyAndSavesWithoutValue)
{
	/*
	 * Asserts: a commonObjects entry may set only rarity; value stays 0 (filled at generation) and
	 * maxPerZone is unlimited. Save writes only the fields that are set.
	 * Pass: scholar loads with value 0, probability 7 and unlimited maxPerZone; saved rmg has only
	 * rarity.
	 * Fail: the entry is rejected, a default value is filled at load, or save writes value or
	 * zoneLimit.
	 */

	auto loaded = loadConfig(R"({ "commonObjects" : [ { "type" : "scholar", "rmg" : { "rarity" : 7 } } ] })");
	const auto * object = findConfigured(loaded, scholar());
	ASSERT_NE(object, nullptr);
	EXPECT_EQ(object->value, 0u);
	EXPECT_EQ(object->probability, 7);
	EXPECT_EQ(object->maxPerZone, static_cast<ui32>(std::numeric_limits<int>::max()));

	JsonNode saved = saveConfig(loaded);
	ASSERT_FALSE(saved["commonObjects"].Vector().empty());
	const JsonNode & rmg = saved["commonObjects"][0]["rmg"];
	EXPECT_TRUE(rmg["value"].isNull());
	EXPECT_EQ(rmg["rarity"].Integer(), 7);
	EXPECT_TRUE(rmg["zoneLimit"].isNull());
}

TEST_F(ObjectConfigTest, CommonObjectLoadsWithoutRmg)
{
	/*
	 * Asserts: a commonObjects entry with only type loads; missing rmg means default value, rarity
	 * and unlimited zoneLimit.
	 * Pass: scholar is present with value 0, probability 0 and unlimited maxPerZone.
	 * Fail: the entry is rejected because rmg is missing.
	 */

	auto loaded = loadConfig(R"({ "commonObjects" : [ { "type" : "scholar" } ] })");
	const auto * object = findConfigured(loaded, scholar());
	ASSERT_NE(object, nullptr);
	EXPECT_EQ(object->value, 0u);
	EXPECT_EQ(object->probability, 0);
	EXPECT_EQ(object->maxPerZone, static_cast<ui32>(std::numeric_limits<int>::max()));
}

TEST_F(ObjectConfigTest, RandomDwellingLvlLoadsWithoutRarityDefault)
{
	/*
	 * Asserts: a randomDwellingLvl entry without rarity keeps probability 0 at load; NATIVE_DWELLING_RARITY
	 * is applied during generation, not here.
	 * Pass: probability is 0 and value is the written 1000.
	 * Fail: probability is filled with NATIVE_DWELLING_RARITY at load.
	 */

	auto loaded = loadConfig(R"({
		"commonObjects" : [
			{ "type" : "randomDwellingLvl", "subtype" : "objectLvl1", "rmg" : { "value" : 1000 } }
		]
	})");
	const auto * object = findConfigured(loaded, randomDwelling(0));
	ASSERT_NE(object, nullptr);
	EXPECT_EQ(object->value, 1000u);
	EXPECT_EQ(object->probability, 0);
}

}
