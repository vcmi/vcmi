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

#include "../../lib/JsonNode.h"
#include "../../lib/filesystem/ResourceID.h"

#include "../../lib/rmg/CRmgTemplate.h"

#include "../../lib/serializer/JsonSerializer.h"
#include "../../lib/serializer/JsonDeserializer.h"

#include "../JsonComparer.h"


namespace test
{
using namespace ::rmg;
using namespace ::testing;

class CRmgTemplateTest : public Test
{
public:
	const std::string TEST_DATA_PATH = "test/rmg/";

protected:

	void testLoadSave(const std::string & id, const JsonNode & config)
	{
		std::shared_ptr<CRmgTemplate> subject = std::make_shared<CRmgTemplate>();
		subject->setId(id);

		{
			JsonDeserializer handler(nullptr, config);
			subject->serializeJson(handler);
		}

		EXPECT_FALSE(subject->getName().empty());

		for(const auto & idAndZone : subject->getZones())
		{
			auto thisZone = idAndZone.second;
			if(thisZone->getMinesLikeZone() != ZoneOptions::NO_ZONE)
			{
				auto otherZoneId = thisZone->getMinesLikeZone();

				const auto otherZone = subject->getZones().at(otherZoneId);
				GTEST_ASSERT_NE(otherZone, nullptr);
				EXPECT_THAT(thisZone->getMinesInfo(), ContainerEq(otherZone->getMinesInfo()));
			}

			if(thisZone->getTerrainTypeLikeZone() != ZoneOptions::NO_ZONE)
			{
				auto otherZoneId = thisZone->getTerrainTypeLikeZone();

				const auto otherZone = subject->getZones().at(otherZoneId);
				GTEST_ASSERT_NE(otherZone, nullptr);
				EXPECT_THAT(thisZone->getTerrainTypes(), ContainerEq(otherZone->getTerrainTypes()));
			}

			if(thisZone->getTreasureLikeZone() != ZoneOptions::NO_ZONE)
			{
				auto otherZoneId = thisZone->getTreasureLikeZone();

				const auto otherZone = subject->getZones().at(otherZoneId);
				GTEST_ASSERT_NE(otherZone, nullptr);
				EXPECT_THAT(thisZone->getTreasureInfo(), ContainerEq(otherZone->getTreasureInfo()));;
			}
		}

		for(const auto & connection : subject->getConnections())
		{
			auto id1 = connection.getZoneA();
			auto id2 = connection.getZoneB();

			auto zone1 = subject->getZones().at(id1);
			auto zone2 = subject->getZones().at(id2);

			EXPECT_THAT(zone1->getConnections(), Contains(id2));
			EXPECT_THAT(zone2->getConnections(), Contains(id1));
		}

		JsonNode actual(JsonNode::JsonType::DATA_STRUCT);

		{
			JsonSerializer handler(nullptr, actual);
			subject->serializeJson(handler);
		}

		JsonComparer cmp(false);
		cmp.compare(id, actual, config);
	}


};

TEST_F(CRmgTemplateTest, SerializeCycle)
{
	const std::string testFilePath = TEST_DATA_PATH + "1.json";
	ResourceID testFileRes(testFilePath);
	JsonNode testData(testFileRes);

	ASSERT_FALSE((testData.Struct().empty()));

	for(const auto & idAndConfig : testData.Struct())
		testLoadSave(idAndConfig.first, idAndConfig.second);
}

}
