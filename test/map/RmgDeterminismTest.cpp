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
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/RmgArea.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/serializer/JsonDeserializer.h"

#include <algorithm>
#include <tbb/global_control.h>

namespace
{
constexpr int TEST_RANDOM_SEED = 1337;
constexpr int TEST_SINGLE_THREAD_PARALLELISM = 1;
constexpr int TEST_PARALLEL_PARALLELISM = 8;
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

std::unique_ptr<CMap> generateMap(int randomSeed, std::time_t creationDateTime, bool singleThread, int parallelism)
{
	tbb::global_control limitParallelism(tbb::global_control::max_allowed_parallelism, parallelism);

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

	// Tests select scheduler mode explicitly per scenario.
	auto & mutableConfig = const_cast<CMapGenerator::Config &>(generator.getConfig());
	mutableConfig.singleThread = singleThread;

	return generator.generate(creationDateTime);
}

std::vector<ui8> serializeMap(const std::unique_ptr<CMap> & map)
{
	CMemoryBuffer output;
	CMapSaverJson saver(&output);
	saver.saveMap(map);
	return output.getBuffer();
}

std::unique_ptr<CMap> generateEditorLikeMap(int randomSeed, std::time_t creationDateTime)
{
	CMapGenOptions options;
	options.setWidth(CMapHeader::MAP_SIZE_XLARGE);
	options.setHeight(CMapHeader::MAP_SIZE_XLARGE);
	options.setLevels(2);
	options.setHumanOrCpuPlayerCount(8);
	options.setCompOnlyPlayerCount(0);
	options.setTeamCount(0);
	options.setCompOnlyTeamCount(CMapGenOptions::RANDOM_SIZE);
	options.setWaterContent(EWaterContent::NONE);
	options.setMonsterStrength(EMonsterStrength::RANDOM);
	options.setRoadEnabled(Road::DIRT_ROAD, true);
	options.setRoadEnabled(Road::GRAVEL_ROAD, true);
	options.setRoadEnabled(Road::COBBLESTONE_ROAD, true);

	CMapGenerator generator(options, nullptr, randomSeed);
	return generator.generate(creationDateTime);
}

std::vector<std::string> collectProhibitedArtifacts(const CMap & map)
{
	std::vector<std::string> result;

	for(const auto & obj : map.objects)
	{
		if(auto * artifact = dynamic_cast<CGArtifact *>(obj.get()))
		{
			if(artifact->ID == Obj::ARTIFACT && map.allowedArtifact.count(artifact->getArtifactType()) == 0)
				result.push_back(artifact->getArtifactType().toEntity(LIBRARY)->getNameTranslated());
		}
	}

	return result;
}
}

TEST(RmgDeterminism, SameSeedProducesSameSerializedMap)
{
	const auto first = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, true, TEST_SINGLE_THREAD_PARALLELISM));
	const auto second = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, true, TEST_SINGLE_THREAD_PARALLELISM));
	EXPECT_EQ(first, second);
}

TEST(RmgDeterminism, CreationTimestampCanBeOverridden)
{
	constexpr std::time_t timestampA = TEST_CREATION_TIME;
	constexpr std::time_t timestampB = TEST_CREATION_TIME + 86400;

	auto mapA = generateMap(TEST_RANDOM_SEED, timestampA, true, TEST_SINGLE_THREAD_PARALLELISM);
	auto mapB = generateMap(TEST_RANDOM_SEED, timestampB, true, TEST_SINGLE_THREAD_PARALLELISM);
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

TEST(RmgDeterminism, DeterministicSeedDerivationIsStable)
{
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

	const int reference = generator.deriveDeterministicSeed(7, "MinePlacer", "objects");
	EXPECT_EQ(reference, generator.deriveDeterministicSeed(7, "MinePlacer", "objects"));
	EXPECT_NE(reference, generator.deriveDeterministicSeed(8, "MinePlacer", "objects"));
	EXPECT_NE(reference, generator.deriveDeterministicSeed(7, "MinePlacer", "extra"));
	EXPECT_NE(reference, 0);
}

TEST(RmgDeterminism, DISABLED_ParallelSameSeedProducesSameSerializedMap)
{
	const auto first = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, false, TEST_PARALLEL_PARALLELISM));
	const auto second = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, false, TEST_PARALLEL_PARALLELISM));
	EXPECT_EQ(first, second);
}

TEST(RmgDeterminism, DISABLED_ParallelResultIsThreadCountInvariant)
{
	const auto baseline = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, false, 1));

	for(const int parallelism : {2, 4, 8})
	{
		const auto candidate = serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, false, parallelism));
		EXPECT_EQ(baseline, candidate);
	}
}

TEST(RmgDeterminism, EditorDefaultsDoNotPlaceProhibitedArtifacts)
{
	const auto map = generateEditorLikeMap(1, TEST_CREATION_TIME);
	const auto prohibited = collectProhibitedArtifacts(*map);

	EXPECT_TRUE(prohibited.empty()) << testing::PrintToString(prohibited);
}
