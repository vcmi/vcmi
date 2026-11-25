/*
 * CRmgTemplatePenroseScaleTest.cpp, part of VCMI engine
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
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/modding/IdentifierResolver.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../../lib/serializer/JsonDeserializer.h"

namespace test
{

namespace
{

float loadPenroseTileScale(const JsonNode * scale)
{
	const std::string json = R"({
		"name" : "scale-test",
		"minSize" : "s",
		"maxSize" : "s",
		"players" : "2",
		"zones" : {
			"1" : { "type" : "playerStart", "size" : 10, "owner" : 1 }
		},
		"connections" : []
	})";
	JsonNode config(json.data(), json.size(), "penroseTileScale.json");
	if(scale)
		config.Struct()["penroseTileScale"] = *scale;

	auto subject = std::make_shared<CRmgTemplate>();
	subject->setId("scale-test");
	InstantIdentifierResolver resolver(*LIBRARY->identifiers());
	JsonDeserializer handler(nullptr, resolver, config);
	subject->serializeJson(handler, *LIBRARY->objtypeh);
	return subject->getPenroseTileScale();
}

}

TEST(CRmgTemplatePenroseScaleTest, DefaultAndClamp)
{
	EXPECT_FLOAT_EQ(loadPenroseTileScale(nullptr), 1.0f);

	const JsonNode inRange(0.5);
	EXPECT_FLOAT_EQ(loadPenroseTileScale(&inRange), 0.5f);

	const JsonNode tooSmall(0.1);
	EXPECT_FLOAT_EQ(loadPenroseTileScale(&tooSmall), 0.25f);

	const JsonNode tooLarge(3.0);
	EXPECT_FLOAT_EQ(loadPenroseTileScale(&tooLarge), 2.0f);
}

}
