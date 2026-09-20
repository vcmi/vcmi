/*
 * ObjectTemplateLayersTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/mapObjects/ObjectTemplate.h"

namespace
{
/// mask lines are written top to bottom, left to right, the same way as in object config
ObjectTemplate makeTemplate(const std::string & mask, int zIndex)
{
	const std::string text = R"({ "animation" : "test.def", "visitableFrom" : [], "mask" : )" + mask + R"(, "zIndex" : )" + std::to_string(zIndex) + " }";

	JsonNode node(text.data(), text.size(), "ObjectTemplateLayersTest");
	ObjectTemplate result;
	result.readJson(node, false);
	return result;
}
}

TEST(ObjectTemplateLayers, LayersRiseWithRows)
{
	// passable top row above blocked bottom row, like a tree
	auto tmpl = makeTemplate(R"([ "VVV", "BBB" ])", 0);

	for(int x = 0; x < 3; ++x)
	{
		EXPECT_EQ(tmpl.getDrawLayerAt(x, 0), 1);
		EXPECT_EQ(tmpl.getDrawLayerAt(x, 1), 2);
	}
}

TEST(ObjectTemplateLayers, BlockedCellRestartsColumn)
{
	// rightmost column has blocked cell on top of passable one, so it restarts at 1.
	// Cell to the left of it is passable and joins the layer of that blocked cell
	auto tmpl = makeTemplate(R"([ "VVB", "VVV" ])", 0);

	EXPECT_EQ(tmpl.getDrawLayerAt(0, 0), 1);
	EXPECT_EQ(tmpl.getDrawLayerAt(0, 1), 1);
	EXPECT_EQ(tmpl.getDrawLayerAt(1, 0), 1);
	EXPECT_EQ(tmpl.getDrawLayerAt(1, 1), 1);
	EXPECT_EQ(tmpl.getDrawLayerAt(2, 0), 1);
	EXPECT_EQ(tmpl.getDrawLayerAt(2, 1), 2);
}

TEST(ObjectTemplateLayers, PriorityObjectsStayOutsideOfLayers)
{
	auto below = makeTemplate(R"([ "VVV", "BBB" ])", 100);
	auto above = makeTemplate(R"([ "VVV", "BBB" ])", -100);

	for(int y = 0; y < 2; ++y)
	{
		for(int x = 0; x < 3; ++x)
		{
			EXPECT_EQ(below.getDrawLayerAt(x, y), 0);
			EXPECT_EQ(above.getDrawLayerAt(x, y), 255);
		}
	}
}

TEST(ObjectTemplateLayers, OutsideOfTemplateIsBottomLayer)
{
	auto tmpl = makeTemplate(R"([ "VVV", "BBB" ])", 0);

	EXPECT_EQ(tmpl.getDrawLayerAt(-1, 0), 0);
	EXPECT_EQ(tmpl.getDrawLayerAt(3, 0), 0);
	EXPECT_EQ(tmpl.getDrawLayerAt(0, 2), 0);
}
