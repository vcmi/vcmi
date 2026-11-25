/*
 * CRoadRandomizerTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/rmg/CRoadRandomizer.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../../lib/serializer/JsonDeserializer.h"

namespace test
{
using namespace ::rmg;
using namespace ::testing;

class CRoadRandomizerTest : public Test
{
protected:
	static constexpr int SEED_COUNT = 20;

	static std::shared_ptr<CRmgTemplate> loadTemplate(const std::string & json)
	{
		JsonNode config(json.data(), json.size(), "test");
		auto subject = std::make_shared<CRmgTemplate>();
		subject->setId("roadRandomizerTest");

		{
			InstantIdentifierResolver resolver(*LIBRARY->identifiers());
			JsonDeserializer handler(nullptr, resolver, config);
			subject->serializeJson(handler, *LIBRARY->objtypeh);
		}

		subject->afterLoad(*LIBRARY);
		return subject;
	}

	static void randomize(const CRmgTemplate & tmpl, int seed)
	{
		CRandomGenerator rng(seed);
		CRoadRandomizer(tmpl.getZones()).dropRandomRoads(rng);
	}

	static ERoadOption roadBetween(const CRmgTemplate & tmpl, TRmgTemplateZoneId a, TRmgTemplateZoneId b)
	{
		for(const auto & connection : tmpl.getZones().at(a)->getConnections())
		{
			if((connection.getZoneA() == a && connection.getZoneB() == b)
				|| (connection.getZoneA() == b && connection.getZoneB() == a))
			{
				return connection.getRoadOption();
			}
		}

		ADD_FAILURE() << "No connection between zones " << a << " and " << b;
		return ERoadOption::ROAD_RANDOM;
	}
};

TEST_F(CRoadRandomizerTest, RepulsivePairDoesNotCarryRoad)
{
	const std::string json = R"({
		"minSize" : "s", "maxSize" : "s",
		"players" : "1",
		"zones" :
		{
			"1" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"2" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"3" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } }
		},
		"connections" :
		[
			{ "a" : "1", "b" : "2" },
			{ "a" : "2", "b" : "3" },
			{ "a" : "1", "b" : "3", "type" : "repulsive" }
		]
	})";

	for(int seed = 0; seed < SEED_COUNT; ++seed)
	{
		SCOPED_TRACE(seed);
		auto tmpl = loadTemplate(json);
		randomize(*tmpl, seed);
		EXPECT_EQ(roadBetween(*tmpl, 1, 2), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(roadBetween(*tmpl, 2, 3), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(roadBetween(*tmpl, 1, 3), ERoadOption::ROAD_FALSE);
	}
}

TEST_F(CRoadRandomizerTest, TownlessChainKeepsRoad)
{
	const std::string json = R"({
		"minSize" : "s", "maxSize" : "s",
		"players" : "1",
		"zones" :
		{
			"1" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"2" : { "type" : "treasure", "size" : 1 },
			"3" : { "type" : "treasure", "size" : 1 },
			"4" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } }
		},
		"connections" :
		[
			{ "a" : "1", "b" : "2" },
			{ "a" : "2", "b" : "3" },
			{ "a" : "3", "b" : "4" }
		]
	})";

	for(int seed = 0; seed < SEED_COUNT; ++seed)
	{
		SCOPED_TRACE(seed);
		auto tmpl = loadTemplate(json);
		randomize(*tmpl, seed);
		EXPECT_EQ(roadBetween(*tmpl, 1, 2), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(roadBetween(*tmpl, 2, 3), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(roadBetween(*tmpl, 3, 4), ERoadOption::ROAD_TRUE);
	}
}

TEST_F(CRoadRandomizerTest, TownlessDeadEndIsTrimmed)
{
	const std::string json = R"({
		"minSize" : "s", "maxSize" : "s",
		"players" : "1",
		"zones" :
		{
			"1" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"2" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"3" : { "type" : "treasure", "size" : 1 }
		},
		"connections" :
		[
			{ "a" : "1", "b" : "2" },
			{ "a" : "2", "b" : "3" }
		]
	})";

	for(int seed = 0; seed < SEED_COUNT; ++seed)
	{
		SCOPED_TRACE(seed);
		auto tmpl = loadTemplate(json);
		randomize(*tmpl, seed);
		EXPECT_EQ(roadBetween(*tmpl, 1, 2), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(roadBetween(*tmpl, 2, 3), ERoadOption::ROAD_FALSE);
	}
}

TEST_F(CRoadRandomizerTest, ExplicitRoadIntoTownlessLeafStays)
{
	const std::string json = R"({
		"minSize" : "s", "maxSize" : "s",
		"players" : "1",
		"zones" :
		{
			"1" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"2" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"3" : { "type" : "treasure", "size" : 1 }
		},
		"connections" :
		[
			{ "a" : "1", "b" : "2" },
			{ "a" : "2", "b" : "3", "road" : "true" }
		]
	})";

	for(int seed = 0; seed < SEED_COUNT; ++seed)
	{
		SCOPED_TRACE(seed);
		auto tmpl = loadTemplate(json);
		randomize(*tmpl, seed);
		EXPECT_EQ(roadBetween(*tmpl, 1, 2), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(roadBetween(*tmpl, 2, 3), ERoadOption::ROAD_TRUE);
	}
}

TEST_F(CRoadRandomizerTest, CycleOfThreeTownsHasExactlyTwoRoads)
{
	const std::string json = R"({
		"minSize" : "s", "maxSize" : "s",
		"players" : "1",
		"zones" :
		{
			"1" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"2" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } },
			"3" : { "type" : "treasure", "size" : 1, "neutralTowns" : { "towns" : 1 } }
		},
		"connections" :
		[
			{ "a" : "1", "b" : "2" },
			{ "a" : "2", "b" : "3" },
			{ "a" : "3", "b" : "1" }
		]
	})";

	for(int seed = 0; seed < SEED_COUNT; ++seed)
	{
		SCOPED_TRACE(seed);
		auto tmpl = loadTemplate(json);
		randomize(*tmpl, seed);

		const auto road12 = roadBetween(*tmpl, 1, 2);
		const auto road23 = roadBetween(*tmpl, 2, 3);
		const auto road31 = roadBetween(*tmpl, 3, 1);
		EXPECT_NE(road12, ERoadOption::ROAD_RANDOM);
		EXPECT_NE(road23, ERoadOption::ROAD_RANDOM);
		EXPECT_NE(road31, ERoadOption::ROAD_RANDOM);

		const std::array roads = {road12, road23, road31};
		const auto trueCount = std::count(roads.begin(), roads.end(), ERoadOption::ROAD_TRUE);
		EXPECT_EQ(trueCount, 2);
	}
}

}
