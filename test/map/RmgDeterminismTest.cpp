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
#include "../../lib/filesystem/CZipLoader.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/MapFormatJson.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/RmgArea.h"
#include "../../lib/ScopeGuard.h"
#include "../../lib/serializer/JsonDeserializer.h"
#include "../mock/mock_IGameInfoCallback.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <tbb/global_control.h>

namespace
{
constexpr int TEST_RANDOM_SEED = 1337;
constexpr int TEST_SINGLE_THREAD_PARALLELISM = 1;
constexpr int TEST_PARALLEL_PARALLELISM = 8;
constexpr std::time_t TEST_CREATION_TIME = 1'725'897'600;
const std::string TEST_TEMPLATE_ID = "2SM2a";
const std::string TEST_TEMPLATE_DATA_PATH = "test/rmg/1.json";
const CMap * gMapForCallbackLookup = nullptr;

std::shared_ptr<CRmgTemplate> loadTemplate()
{
	JsonNode testData(JsonPath::builtin(TEST_TEMPLATE_DATA_PATH));
	testData.setModScope(ModScope::scopeBuiltin(), true);

	auto result = std::make_shared<CRmgTemplate>();
	result->setId(TEST_TEMPLATE_ID);

	JsonDeserializer handler(nullptr, testData[TEST_TEMPLATE_ID]);
	result->serializeJson(handler);
	result->afterLoad();
	result->validate();

	return result;
}

IGameInfoCallbackMock & getDummyCallback()
{
	static ::testing::NiceMock<IGameInfoCallbackMock> callback;
	static bool configured = false;

	if(!configured)
	{
		ON_CALL(callback, isAllowed(::testing::An<ArtifactID>()))
			.WillByDefault(::testing::Return(true));
		ON_CALL(callback, getArtInstance(::testing::_))
			.WillByDefault([](ArtifactInstanceID aid) -> const CArtifactInstance *
		{
			return gMapForCallbackLookup ? gMapForCallbackLookup->getArtifactInstance(aid) : nullptr;
		});
		configured = true;
	}

	return callback;
}

std::unique_ptr<CMap> generateMap(int randomSeed, std::time_t creationDateTime, bool singleThread, int parallelism)
{
	const int effectiveParallelism = singleThread ? 1 : parallelism;
	tbb::global_control limitParallelism(tbb::global_control::max_allowed_parallelism, effectiveParallelism);

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
	auto & callback = getDummyCallback();
	CMapGenerator generator(options, &callback, randomSeed);
	generator.setSingleThread(singleThread);

	auto map = generator.generate(creationDateTime);
	gMapForCallbackLookup = map.get();
	return map;
}

std::vector<ui8> serializeMap(std::unique_ptr<CMap> map)
{
	gMapForCallbackLookup = map.get();

	CMemoryBuffer output;
	CMapSaverJson saver(&output);
	saver.saveMap(map);
	return output.getBuffer();
}

std::map<std::string, std::string> extractArchivePayload(const std::vector<ui8> & serializedMap)
{
	std::map<std::string, std::string> payloadByName;

	CMemoryBuffer input;
	const auto written = input.write(serializedMap.data(), static_cast<si64>(serializedMap.size()));
	if(written != static_cast<si64>(serializedMap.size()))
		throw std::runtime_error("Failed to stage serialized map for determinism test.");

	input.seek(0);
	std::shared_ptr<CIOApi> ioApi(new CProxyROIOApi(&input));
	CZipLoader archive("", "_", ioApi);

	const auto files = archive.getFilteredFiles([](const ResourcePath &)
	{
		return true;
	});

	for(const auto & file : files)
	{
		auto stream = archive.load(file);
		if(!stream)
			continue;

		auto data = stream->readAll();
		if(!data.first)
			throw std::runtime_error("Failed to read unpacked map payload in determinism test.");

		payloadByName.emplace(file.getOriginalName(), std::string(reinterpret_cast<const char *>(data.first.get()), data.second));
	}

	return payloadByName;
}
}

TEST(RmgDeterminism, SameSeedProducesSameSerializedMap)
{
	const auto first = extractArchivePayload(serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, true, TEST_SINGLE_THREAD_PARALLELISM)));
	const auto second = extractArchivePayload(serializeMap(generateMap(TEST_RANDOM_SEED, TEST_CREATION_TIME, true, TEST_SINGLE_THREAD_PARALLELISM)));
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

	const auto serializedA = serializeMap(std::move(mapA));
	const auto serializedB = serializeMap(std::move(mapB));
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
