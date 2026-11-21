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
	 * Asserts: afterLoad concatenates template pandoraBoxes with zone boxes, and
	 * customObjectsLikeZone copies the effective list.
	 * Pass: zones 1 and 2 each have two boxes, template 1000 then zone 2000.
	 * Fail: zone 2 is empty, or the template box is missing, or the order is reversed.
	 */

	constexpr char text[] = R"({
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "1",
		"customObjects" : {
			"pandoraBoxes" : [
				{ "reward" : { "heroExperience" : 1000 } }
			]
		},
		"zones" : {
			"1" : {
				"type" : "treasure",
				"size" : 10,
				"customObjects" : {
					"pandoraBoxes" : [
						{ "reward" : { "heroExperience" : 2000 } }
					]
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
	subject->setId("pandora");

	{
		InstantIdentifierResolver resolver(*LIBRARY->identifiers());
		JsonDeserializer handler(nullptr, resolver, config);
		subject->serializeJson(handler, *LIBRARY->objtypeh);
	}

	subject->afterLoad(*LIBRARY);

	const auto & zoneA = subject->getZones().at(1)->getCustomObjects();
	ASSERT_EQ(zoneA.getPandoraBoxes().size(), 2u);
	EXPECT_EQ(zoneA.getPandoraBoxes()[0].reward.heroExperience, 1000);
	EXPECT_EQ(zoneA.getPandoraBoxes()[1].reward.heroExperience, 2000);

	const auto & zoneB = subject->getZones().at(2)->getCustomObjects();
	ASSERT_EQ(zoneB.getPandoraBoxes().size(), 2u);
	EXPECT_EQ(zoneB.getPandoraBoxes()[0].reward.heroExperience, 1000);
	EXPECT_EQ(zoneB.getPandoraBoxes()[1].reward.heroExperience, 2000);
}

}
