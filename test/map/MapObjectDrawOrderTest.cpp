/*
 * MapObjectDrawOrderTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/MapObjectDrawOrder.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"

namespace
{
const int3 tile(5, 5, 0);

/// mask lines are written top to bottom, like in object config
std::shared_ptr<ObjectTemplate> makeTemplate(const std::string & mask, int zIndex)
{
	const std::string text = R"({ "animation" : "test.def", "visitableFrom" : [], "mask" : )" + mask + R"(, "zIndex" : )" + std::to_string(zIndex) + " }";

	JsonNode node(text.data(), text.size(), "MapObjectDrawOrderTest");
	auto result = std::make_shared<ObjectTemplate>();
	result->readJson(node, false);
	return result;
}

class MapObjectDrawOrderTest : public ::testing::Test
{
protected:
	CMap map{nullptr};
	std::vector<std::unique_ptr<CGObjectInstance>> objects;

	/// The name is the letter of the object in the result of draw(): capital for the body, small for the shadow
	const CGObjectInstance * addObject(char name, const std::string & mask, const int3 & anchor, int zIndex = 0, MapObjectID type = Obj::NO_OBJ)
	{
		auto object = std::make_unique<CGObjectInstance>(nullptr);
		object->id = ObjectInstanceID(name);
		object->ID = type;
		object->appearance = makeTemplate(mask, zIndex);
		object->pos = anchor;
		objects.push_back(std::move(object));
		return objects.back().get();
	}

	/// Objects are put on the tile one after another, like H3 does when it loads the map
	std::string draw(const std::vector<const CGObjectInstance *> & placed, int slotOffsetY = 0)
	{
		std::vector<const CGObjectInstance *> onTile;

		for(const auto * object : placed)
			onTile.insert(MapObjectDrawOrder::findInsertPosition(onTile, map, object, tile, [](const CGObjectInstance * entry) { return entry; }), object);

		std::string result;

		for(const auto & step : MapObjectDrawOrder::getDrawSteps(onTile, tile, [](const CGObjectInstance * entry) { return entry; }, [&](const CGObjectInstance *) { return Point(0, slotOffsetY); }))
		{
			const char name = static_cast<char>(step.object->id.getNum());
			result += step.isShadow ? static_cast<char>(std::tolower(name)) : name;
		}
		return result;
	}
};
}

// Tree-like object: passable top row, blocked bottom row. The crown of the tree is on the tile above the trunk
TEST_F(MapObjectDrawOrderTest, TreeCrownCoversObjectBehindIt)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * rock = addObject('R', R"([ "B" ])", int3(5, 5, 0));

	// whichever of them is loaded first
	EXPECT_EQ(draw({tree, rock}), "rtRT");
	EXPECT_EQ(draw({rock, tree}), "rtRT");
}

TEST_F(MapObjectDrawOrderTest, EqualObjectsKeepLoadOrderWhenAddedAgain)
{
	const auto * first = addObject('A', R"([ "B" ])", int3(5, 5, 0));
	const auto * second = addObject('B', R"([ "B" ])", int3(5, 5, 0));

	EXPECT_EQ(draw({first, second}), "abAB");
	EXPECT_EQ(draw({second, first}), "abAB");
}

TEST_F(MapObjectDrawOrderTest, BackgroundObjectIsBelowEverythingAndItsShadowToo)
{
	const auto * ground = addObject('G', R"([ "B" ])", int3(5, 5, 0), 100);
	const auto * rock = addObject('R', R"([ "B" ])", int3(5, 5, 0));

	// shadows go after the bottom layer, so that they never darken the objects lying on the ground
	EXPECT_EQ(draw({ground, rock}), "GgrR");
	EXPECT_EQ(draw({rock, ground}), "GgrR");
}

TEST_F(MapObjectDrawOrderTest, ForegroundObjectIsAboveEverything)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * overlay = addObject('O', R"([ "B" ])", int3(5, 5, 0), -100);

	EXPECT_EQ(draw({overlay, tree}), "toTO");
	EXPECT_EQ(draw({tree, overlay}), "toTO");
}

TEST_F(MapObjectDrawOrderTest, HeroWalksBehindTreeCrownButInFrontOfObjectsBehindHim)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * rock = addObject('R', R"([ "B" ])", int3(5, 5, 0));
	const auto * hero = addObject('H', R"([ "B" ])", int3(5, 5, 0), 0, Obj::HERO);

	// hero has no shadow of its own here and stands between the layers of the tile
	EXPECT_EQ(draw({rock, tree, hero}), "rtRHT");
}

TEST_F(MapObjectDrawOrderTest, TopOfHeroIsBehindTreeCrown)
{
	const auto * tree = addObject('T', R"([ "V", "B" ])", int3(5, 6, 0));
	const auto * hero = addObject('H', R"([ "B" ])", int3(5, 5, 0), 0, Obj::HERO);

	// the tile is above the row on which the hero stands, so only his head reaches it
	EXPECT_EQ(draw({tree, hero}, MapObjectDrawOrder::fixedSlotBodyRowLimit), "tTH");
}
