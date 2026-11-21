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

}
