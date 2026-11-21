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

	void testLoadSave(const std::string & id, const JsonNode & config)
	{
		auto subject = std::make_shared<CRmgTemplate>();
		subject->setId(id);

		{
			InstantIdentifierResolver resolver(*LIBRARY->identifiers());
			JsonDeserializer handler(nullptr, resolver, config);
			subject->serializeJson(handler, *LIBRARY->objtypeh);
		}

		if(!config["name"].String().empty())
			EXPECT_EQ(subject->getName(), config["name"].String());

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

}
