/*
 * RmgDeterminismTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/filesystem/CMemoryBuffer.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/serializer/JsonDeserializer.h"

#include <tbb/global_control.h>

namespace
{
constexpr int TEST_RANDOM_SEED = 1337;
constexpr int TEST_PARALLELISM = 1;
const std::string TEST_TEMPLATE_ID = "2SM2a";
const std::string TEST_TEMPLATE_DATA_PATH = "test/rmg/1.json";

std::shared_ptr<CRmgTemplate> loadTemplate()
{
	const JsonNode testData(JsonPath::builtin(TEST_TEMPLATE_DATA_PATH));
	auto result = std::make_shared<CRmgTemplate>();
	result->setId(TEST_TEMPLATE_ID);

	JsonDeserializer handler(nullptr, testData[TEST_TEMPLATE_ID]);
	result->serializeJson(handler);

	return result;
}

std::vector<ui8> generateAndSerializeMap()
{
	tbb::global_control limitParallelism(tbb::global_control::max_allowed_parallelism, TEST_PARALLELISM);

	auto mapTemplate = loadTemplate();
	CMapGenOptions options;
	options.setMapTemplate(mapTemplate.get());
	options.setWidth(CMapHeader::MAP_SIZE_SMALL);
	options.setHeight(CMapHeader::MAP_SIZE_SMALL);
	options.setLevels(1);
	options.setHumanOrCpuPlayerCount(2);
	options.setCompOnlyPlayerCount(0);
	options.setPlayerTypeForStandardPlayer(PlayerColor(0), EPlayerType::HUMAN);
	options.setPlayerTypeForStandardPlayer(PlayerColor(1), EPlayerType::AI);
	CMapGenerator generator(options, nullptr, TEST_RANDOM_SEED);
	auto map = generator.generate();

	CMemoryBuffer output;
	CMapSaverJson saver(&output);
	saver.saveMap(map);
	return output.getBuffer();
}
}

TEST(RmgDeterminism, DISABLED_SameSeedProducesSameSerializedMap)
{
	const auto first = generateAndSerializeMap();
	const auto second = generateAndSerializeMap();
	EXPECT_EQ(first, second);
}
