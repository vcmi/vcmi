/*
 * RmgObjectTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/rmg/Functions.h"
#include "../../lib/rmg/RmgObject.h"

namespace
{
std::shared_ptr<ObjectTemplate> makeTemplate(const std::string & mask)
{
	const std::string text = R"({ "animation" : "test.def", "visitableFrom" : [], "mask" : )" + mask + " }";
	JsonNode node(text.data(), text.size(), "RmgObjectTest");
	auto result = std::make_shared<ObjectTemplate>();
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	result->readJson(node, resolver, false);
	return result;
}

rmg::Object makeObject(const std::string & mask, const int3 & anchor)
{
	auto object = std::make_shared<CGObjectInstance>(nullptr);
	object->ID = Obj::OASIS;
	object->subID = 0;
	object->appearance = makeTemplate(mask);
	return rmg::Object(object, anchor);
}

std::set<int3> tilesOf(const rmg::Area & area)
{
	const auto & tiles = area.getTilesVector();
	return {tiles.begin(), tiles.end()};
}

bool onMapNonNegative(const int3 & tile)
{
	return tile.x >= 0 && tile.y >= 0 && tile.z >= 0;
}
}

TEST(RmgObjectTest, oasisMaskHasSixEntrances)
{
	const auto object = makeObject(R"([ "BB", "AA" ])", int3(10, 10, 0));

	const std::set<int3> expected = {
		int3(8, 10, 0), int3(11, 10, 0),
		int3(8, 11, 0), int3(9, 11, 0), int3(10, 11, 0), int3(11, 11, 0)
	};
	EXPECT_EQ(tilesOf(object.getAccessibleArea()), expected);
}

TEST(RmgObjectTest, wateringHoleMaskHasEightEntrances)
{
	const auto object = makeObject(R"([ "AAAA" ])", int3(10, 10, 0));

	const std::set<int3> expected = {
		int3(6, 10, 0), int3(11, 10, 0),
		int3(6, 11, 0), int3(7, 11, 0), int3(8, 11, 0), int3(9, 11, 0), int3(10, 11, 0), int3(11, 11, 0)
	};
	EXPECT_EQ(tilesOf(object.getAccessibleArea()), expected);
}

TEST(RmgObjectTest, singleTileObjectKeepsFiveEntrances)
{
	const auto object = makeObject(R"([ "A" ])", int3(10, 10, 0));

	const std::set<int3> expected = {
		int3(9, 10, 0), int3(11, 10, 0),
		int3(9, 11, 0), int3(10, 11, 0), int3(11, 11, 0)
	};
	EXPECT_EQ(tilesOf(object.getAccessibleArea()), expected);
}

TEST(RmgObjectTest, oasisVisitableAreaHoldsBothTiles)
{
	const auto object = makeObject(R"([ "BB", "AA" ])", int3(10, 10, 0));

	const std::set<int3> expected = { int3(9, 10, 0), int3(10, 10, 0) };
	EXPECT_EQ(tilesOf(object.getVisitableArea()), expected);
}

TEST(RmgObjectTest, multiEntranceObjectRejectedWhenEntranceIsOffMap)
{
	const auto offMap = makeObject(R"([ "BB", "AA" ])", int3(1, 10, 0));
	EXPECT_FALSE(offMap.allEntrancesOnMap(onMapNonNegative));

	const auto onMap = makeObject(R"([ "BB", "AA" ])", int3(2, 10, 0));
	EXPECT_TRUE(onMap.allEntrancesOnMap(onMapNonNegative));
}

TEST(RmgObjectTest, singleEntranceObjectMayHaveOffMapEntrance)
{
	const auto object = makeObject(R"([ "A" ])", int3(0, 10, 0));
	EXPECT_TRUE(object.allEntrancesOnMap(onMapNonNegative));
}

TEST(RmgObjectTest, oasisFootprintHoldsBodyAndEntrances)
{
	const auto object = makeObject(R"([ "BB", "AA" ])", int3(10, 10, 0));

	const std::set<int3> expected = {
		int3(9, 9, 0), int3(10, 9, 0), int3(9, 10, 0), int3(10, 10, 0),
		int3(8, 10, 0), int3(11, 10, 0),
		int3(8, 11, 0), int3(9, 11, 0), int3(10, 11, 0), int3(11, 11, 0)
	};
	EXPECT_EQ(tilesOf(object.getFootprint()), expected);
	EXPECT_TRUE(object.hasMultipleEntrances());
	EXPECT_FALSE(makeObject(R"([ "A" ])", int3(10, 10, 0)).hasMultipleEntrances());
}

TEST(RmgObjectTest, pileOnOasisEntranceBreaksMinimumDistance)
{
	// A one-tile treasure directly below the Oasis' visitable row stands on one of its entrances
	const auto oasis = makeObject(R"([ "BB", "AA" ])", int3(10, 10, 0));
	const auto onEntrance = makeObject(R"([ "A" ])", int3(10, 11, 0));

	EXPECT_FALSE(footprintKeepsDistance(onEntrance.getFootprint(), oasis.getFootprint(), 1.f));
	EXPECT_FALSE(footprintKeepsDistance(oasis.getFootprint(), onEntrance.getFootprint(), 1.f));
}

TEST(RmgObjectTest, minimumDistanceIsMeasuredBetweenFootprints)
{
	// Body two rows below the visitable row: the footprints touch but do not overlap
	const auto oasis = makeObject(R"([ "BB", "AA" ])", int3(10, 10, 0));
	const auto below = makeObject(R"([ "A" ])", int3(10, 12, 0));

	EXPECT_TRUE(footprintKeepsDistance(below.getFootprint(), oasis.getFootprint(), 1.f));
	EXPECT_FALSE(footprintKeepsDistance(below.getFootprint(), oasis.getFootprint(), 2.f));
}

TEST(RmgObjectTest, nothingPlacedKeepsDistance)
{
	const auto oasis = makeObject(R"([ "BB", "AA" ])", int3(10, 10, 0));
	EXPECT_TRUE(footprintKeepsDistance(oasis.getFootprint(), rmg::Area(), 100.f));
}
