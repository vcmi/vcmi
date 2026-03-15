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
#include "../../lib/rmg/RmgArea.h"
#include "../../lib/serializer/JsonDeserializer.h"

#include <algorithm>
#include <tbb/global_control.h>

namespace
{
constexpr int TEST_RANDOM_SEED = 1337;
constexpr int TEST_PARALLELISM = 1;
constexpr std::time_t TEST_CREATION_TIME = 1'725'897'600;
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

std::unique_ptr<CMap> generateMap(int randomSeed, std::time_t creationDateTime)
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
	CMapGenerator generator(options, nullptr, randomSeed);

	// Force deterministic execution mode in this test suite so map comparison
	// checks focus on ordering behavior, not known parallel scheduling races.
	auto & mutableConfig = const_cast<CMapGenerator::Config &>(generator.getConfig());
	mutableConfig.singleThread = true;

	return generator.generate(creationDateTime);
}

std::vector<ui8> serializeMap(const std::unique_ptr<CMap> & map)
{
	CMemoryBuffer output;
	CMapSaverJson saver(&output);
	saver.saveMap(map);
	return output.getBuffer();
}
}

TEST(RmgDeterminism, SameSeedProducesSameSerializedMap)
{
	const auto first = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME));
	const auto second = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME));
	EXPECT_EQ(first, second);
}

TEST(RmgDeterminism, CreationTimestampCanBeOverridden)
{
	constexpr std::time_t timestampA = TEST_CREATION_TIME;
	constexpr std::time_t timestampB = TEST_CREATION_TIME + 86400;

	auto mapA = generateMap(TEST_RANDOM_SEED, timestampA);
	auto mapB = generateMap(TEST_RANDOM_SEED, timestampB);
	EXPECT_EQ(mapA->creationDateTime, timestampA);
	EXPECT_EQ(mapB->creationDateTime, timestampB);

	const auto serializedA = serializeMap(mapA);
	const auto serializedB = serializeMap(mapB);
	EXPECT_NE(serializedA, serializedB);
}

TEST(RmgDeterminism, AreaTilesVectorIsSorted)
{
	rmg::Tileset tiles;
	tiles.insert(int3(5, 1, 0));
	tiles.insert(int3(2, 3, 0));
	tiles.insert(int3(4, 1, 0));
	tiles.insert(int3(2, 2, 0));

	rmg::Area area(std::move(tiles));
	const auto & vectorView = area.getTilesVector();
	EXPECT_TRUE(std::is_sorted(vectorView.begin(), vectorView.end()));
	EXPECT_EQ(vectorView.front(), int3(4, 1, 0));
	EXPECT_EQ(vectorView.back(), int3(2, 3, 0));
}
