/*
 * CRmgTemplateTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/constants/EntityIdentifiers.h"

#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/rmg/CRmgTemplate.h"

#include "../../lib/serializer/JsonSerializer.h"
#include "../../lib/serializer/JsonDeserializer.h"

#include "../JsonComparer.h"


namespace test
{
using namespace ::testing;

class CRmgTemplateTest : public Test
{
public:
	const std::string TEST_DATA_PATH = "test/rmg/";

protected:

	static void testLoadSave(const std::string & id, const JsonNode & config)
	{
		auto subject = std::make_shared<CRmgTemplate>();
		subject->setId(id);

		// Identifier requests need a mod scope; mod loading sets it, a test file does not
		JsonNode scopedConfig = config;
		scopedConfig.setModScope(ModScope::scopeBuiltin());

		{
			InstantIdentifierResolver resolver(*LIBRARY->identifiers());
			JsonDeserializer handler(nullptr, resolver, scopedConfig);
			subject->serializeJson(handler, *LIBRARY->objtypeh);
		}

		if(!config["name"].String().empty())
		{
			EXPECT_EQ(subject->getName(), config["name"].String());
		}

		JsonNode actual;

		{
			JsonSerializer handler(nullptr, actual);
			subject->serializeJson(handler, *LIBRARY->objtypeh);
		}

		JsonComparer cmp(false);
		cmp.compare(id, actual, config);
	}


};

TEST_F(CRmgTemplateTest, SerializeCycle)
{
	const std::string testFilePath = TEST_DATA_PATH + "1.json";
	JsonPath testFileRes = JsonPath::builtin(testFilePath);
	JsonNode testData(testFileRes);

	ASSERT_FALSE((testData.Struct().empty()));

	for(const auto & idAndConfig : testData.Struct())
		testLoadSave(idAndConfig.first, idAndConfig.second);
}

TEST_F(CRmgTemplateTest, AfterLoadMergesTemplateCustomObjects)
{
	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"customObjects" : {
			"bannedCategories" : ["creatureBank"],
			"bannedObjects" : { "scholar" : true }
		},
		"zones" : {
			"1" : { "type" : "treasure", "size" : 10 },
			"2" : {
				"type" : "treasure",
				"size" : 10,
				"customObjects" : {
					"bannedObjects" : { "scholar" : false, "creatureBank" : false }
				}
			},
			"3" : {
				"type" : "treasure",
				"size" : 10,
				"customObjectsLikeZone" : 2
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 },
			{ "a" : "2", "b" : "3", "guard" : 1000 }
		]
	})";

	JsonNode config(text, std::size(text), "AfterLoadMergesTemplateCustomObjects");
	config.setModScope(ModScope::scopeBuiltin());
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("merge");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	const CompoundMapObjectID scholar(Obj::SCHOLAR, 0);
	const CompoundMapObjectID bank(Obj::CREATURE_BANK, 0);

	const auto & zoneA = subject->getZones().at(1)->getCustomObjects();
	EXPECT_TRUE(zoneA.isObjectBanned(scholar));
	EXPECT_TRUE(zoneA.isObjectBanned(bank));

	const auto & zoneB = subject->getZones().at(2)->getCustomObjects();
	EXPECT_FALSE(zoneB.isObjectBanned(scholar));
	EXPECT_FALSE(zoneB.isObjectBanned(bank));

	const auto & zoneC = subject->getZones().at(3)->getCustomObjects();
	EXPECT_FALSE(zoneC.isObjectBanned(scholar));
	EXPECT_FALSE(zoneC.isObjectBanned(bank));
}

TEST_F(CRmgTemplateTest, FalseBanSurvivesRoundTrip)
{
	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"customObjects" : {
			"bannedCategories" : ["all"],
			"bannedObjects" : { "scholar" : false }
		},
		"zones" : {
			"1" : { "type" : "treasure", "size" : 10 }
		},
		"connections" : []
	})";

	JsonNode config(text, std::size(text), "FalseBanSurvivesRoundTrip");
	config.setModScope(ModScope::scopeBuiltin());
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("falseBan");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	JsonNode actual;
	{
		JsonSerializer handler(nullptr, actual);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	const JsonNode & scholarBan = actual["customObjects"]["bannedObjects"]["core:scholar"];
	ASSERT_TRUE(scholarBan.isBool());
	EXPECT_FALSE(scholarBan.Bool());

	actual.setModScope(ModScope::scopeBuiltin());
	auto reloaded = std::make_shared<CRmgTemplate>();
	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, actual);
		reloaded->serializeJson(handler, *LIBRARY->objtypeh);
	}

	EXPECT_FALSE(reloaded->getCustomObjects().isObjectBanned(CompoundMapObjectID(Obj::SCHOLAR, 0)));
}

TEST_F(CRmgTemplateTest, AfterLoadConcatenatesPandoraBoxes)
{
	/*
	 * Asserts: after afterLoad, a zone's pandoraBoxes are the template-level boxes followed by the
	 * zone's own boxes, and customObjectsLikeZone copies that merged list.
	 * Pass: zones 1 and 2 each hold exactly [root experience box, zone 1 spell box].
	 * Fail: the zone list replaces the template list, the order is reversed, or zone 2 does not
	 * inherit zone 1's boxes.
	 */

	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"customObjects" : {
			"pandoraBoxes" : [ { "experience" : 5000 } ]
		},
		"zones" : {
			"1" : {
				"type" : "treasure",
				"size" : 10,
				"customObjects" : {
					"bannedCategories" : [ "pandorasBox" ],
					"pandoraBoxes" : [ { "spells" : [ "fly" ], "rmg" : { "value" : 8000, "rarity" : 2 } } ]
				}
			},
			"2" : {
				"type" : "treasure",
				"size" : 10,
				"customObjectsLikeZone" : 1
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 }
		]
	})";

	JsonNode config(text, std::size(text), "AfterLoadConcatenatesPandoraBoxes");
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("pandoraBoxes");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	const JsonNode & rootBox = config["customObjects"]["pandoraBoxes"][0];
	const JsonNode & zoneBox = config["zones"]["1"]["customObjects"]["pandoraBoxes"][0];

	EXPECT_THAT(subject->getZones().at(1)->getCustomObjects().getPandoraBoxes(), ElementsAre(rootBox, zoneBox));
	EXPECT_THAT(subject->getZones().at(2)->getCustomObjects().getPandoraBoxes(), ElementsAre(rootBox, zoneBox));
}

TEST_F(CRmgTemplateTest, AfterLoadConcatenatesSeerHuts)
{
	/*
	 * Asserts: after afterLoad, a zone's seerHuts are the template-level huts followed by the
	 * zone's own huts, and customObjectsLikeZone copies that merged list.
	 * Pass: zones 1 and 2 each hold exactly [root experience hut, zone 1 spell hut].
	 * Fail: the zone list replaces the template list, the order is reversed, or zone 2 does not
	 * inherit zone 1's huts.
	 */

	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"customObjects" : {
			"seerHuts" : [ { "experience" : 7000 } ]
		},
		"zones" : {
			"1" : {
				"type" : "treasure",
				"size" : 10,
				"customObjects" : {
					"bannedCategories" : [ "seerHut" ],
					"seerHuts" : [ { "spells" : [ "fly" ], "rmg" : { "value" : 8000, "rarity" : 2 } } ]
				}
			},
			"2" : {
				"type" : "treasure",
				"size" : 10,
				"customObjectsLikeZone" : 1
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 }
		]
	})";

	JsonNode config(text, std::size(text), "AfterLoadConcatenatesSeerHuts");
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("seerHuts");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	const JsonNode & rootHut = config["customObjects"]["seerHuts"][0];
	const JsonNode & zoneHut = config["zones"]["1"]["customObjects"]["seerHuts"][0];

	EXPECT_THAT(subject->getZones().at(1)->getCustomObjects().getSeerHuts(), ElementsAre(rootHut, zoneHut));
	EXPECT_THAT(subject->getZones().at(2)->getCustomObjects().getSeerHuts(), ElementsAre(rootHut, zoneHut));
}

TEST_F(CRmgTemplateTest, AfterLoadMergesTemplateMonsterJoining)
{
	/*
	 * Asserts: after afterLoad, each zone's monsterJoining is the per-field merge of the
	 * template block into the zone block. custom without aggression resets character.
	 * Pass: zone 1 has the template fields; zone 2 keeps them and sets character; zone 3
	 * has character unset.
	 * Fail: zone values replace the whole block, template fields are dropped, or custom
	 * without aggression stays CUSTOM.
	 */
	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"monsterJoining" : {
			"joiningPercentage" : 50,
			"joinOnlyForMoney" : true
		},
		"zones" : {
			"1" : { "type" : "treasure", "size" : 10 },
			"2" : {
				"type" : "treasure",
				"size" : 10,
				"monsterJoining" : { "character" : "hostile" }
			},
			"3" : {
				"type" : "treasure",
				"size" : 10,
				"monsterJoining" : { "character" : "custom" }
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 },
			{ "a" : "2", "b" : "3", "guard" : 1000 }
		]
	})";

	JsonNode config(text, std::size(text), "AfterLoadMergesTemplateMonsterJoining");
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("monsterJoining");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	const auto zoneA = subject->getZones().at(1)->getMonsterJoining();
	EXPECT_FALSE(zoneA.character.has_value());
	EXPECT_EQ(zoneA.joiningPercentage, static_cast<int8_t>(50));
	EXPECT_EQ(zoneA.joinOnlyForMoney, true);

	const auto zoneB = subject->getZones().at(2)->getMonsterJoining();
	EXPECT_EQ(zoneB.character, CGCreature::Character::HOSTILE);
	EXPECT_EQ(zoneB.joiningPercentage, static_cast<int8_t>(50));
	EXPECT_EQ(zoneB.joinOnlyForMoney, true);

	const auto zoneC = subject->getZones().at(3)->getMonsterJoining();
	EXPECT_FALSE(zoneC.character.has_value());
	EXPECT_FALSE(zoneC.aggression.has_value());
	EXPECT_EQ(zoneC.joiningPercentage, static_cast<int8_t>(50));
	EXPECT_EQ(zoneC.joinOnlyForMoney, true);
}

TEST_F(CRmgTemplateTest, TownHintsKeepOnlyTheGivenFieldsAcrossRoundTrip)
{
	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"zones" : {
			"1" : { "type" : "treasure", "size" : 10 },
			"2" : { "type" : "treasure", "size" : 10 },
			"3" : {
				"type" : "treasure",
				"size" : 10,
				"townHints" : [
					{ "relatedToZoneTerrain" : 2 },
					{ "notLikeZone" : [1, 2] },
					{ "notLikeZone" : 1 },
					{ "likeZone" : 1 }
				]
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 },
			{ "a" : "2", "b" : "3", "guard" : 1000 }
		]
	})";

	const auto load = [](JsonNode config)
	{
		config.setModScope(ModScope::scopeBuiltin());
		auto subject = std::make_shared<CRmgTemplate>();
		subject->setId("townHints");
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
		return subject;
	};

	const auto expectHints = [](const CRmgTemplate & tpl)
	{
		const auto & hints = tpl.getZones().at(3)->getTownHints();
		ASSERT_EQ(hints.size(), 4u);

		EXPECT_EQ(hints[0].relatedToZoneTerrain, 2);
		EXPECT_THAT(hints[0].notLikeZone, IsEmpty());
		EXPECT_EQ(hints[0].likeZone, rmg::ZoneOptions::NO_ZONE);

		EXPECT_THAT(hints[1].notLikeZone, ElementsAre(1, 2));
		EXPECT_THAT(hints[2].notLikeZone, ElementsAre(1));

		EXPECT_EQ(hints[3].likeZone, 1);
		EXPECT_THAT(hints[3].notLikeZone, IsEmpty());
		EXPECT_EQ(hints[3].relatedToZoneTerrain, rmg::ZoneOptions::NO_ZONE);
	};

	const auto subject = load(JsonNode(text, std::size(text), "TownHintsKeepOnlyTheGivenFieldsAcrossRoundTrip"));
	expectHints(*subject);

	JsonNode saved;
	{
		JsonSerializer handler(nullptr, saved);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	expectHints(*load(saved));
}

TEST_F(CRmgTemplateTest, AfterLoadInheritsTemplateTownsIgnoreGuards)
{
	/*
	 * Asserts: after afterLoad, a zone without townsIgnoreGuards takes the template
	 * value, an explicit zone value overrides it, and townsLikeZone copies the source.
	 * Pass: zone 1 is true, zone 2 is false, zone 3 copies zone 2 and is false.
	 * Fail: unset zones stay false, explicit false is treated as absent, or townsLikeZone
	 * keeps the zone's own value.
	 */
	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"townsIgnoreGuards" : true,
		"zones" : {
			"1" : { "type" : "treasure", "size" : 10 },
			"2" : {
				"type" : "treasure",
				"size" : 10,
				"townsIgnoreGuards" : false
			},
			"3" : {
				"type" : "treasure",
				"size" : 10,
				"townsIgnoreGuards" : true,
				"townsLikeZone" : 2
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 },
			{ "a" : "2", "b" : "3", "guard" : 1000 }
		]
	})";

	JsonNode config(text, std::size(text), "AfterLoadInheritsTemplateTownsIgnoreGuards");
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("townsIgnoreGuards");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	EXPECT_TRUE(subject->getZones().at(1)->getTownsIgnoreGuards());
	EXPECT_FALSE(subject->getZones().at(2)->getTownsIgnoreGuards());
	EXPECT_FALSE(subject->getZones().at(3)->getTownsIgnoreGuards());
}

TEST_F(CRmgTemplateTest, AfterLoadDefaultTownsIgnoreGuardsIsFalse)
{
	/*
	 * Asserts: after afterLoad, a template without townsIgnoreGuards leaves unset
	 * zones false, and an explicit zone true is kept.
	 * Pass: zone 1 is false, zone 2 is true.
	 * Fail: missing template key is treated as true, or the zone key is ignored.
	 */
	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"zones" : {
			"1" : { "type" : "treasure", "size" : 10 },
			"2" : {
				"type" : "treasure",
				"size" : 10,
				"townsIgnoreGuards" : true
			}
		},
		"connections" : [
			{ "a" : "1", "b" : "2", "guard" : 1000 }
		]
	})";

	JsonNode config(text, std::size(text), "AfterLoadDefaultTownsIgnoreGuardsIsFalse");
	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("townsIgnoreGuardsDefault");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	EXPECT_FALSE(subject->getZones().at(1)->getTownsIgnoreGuards());
	EXPECT_TRUE(subject->getZones().at(2)->getTownsIgnoreGuards());
}

}
