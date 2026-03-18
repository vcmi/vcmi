/*
 * RmgFuzzSpec.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "RmgFuzzSpec.h"

#include "../lib/json/JsonNode.h"
#include "../lib/mapping/CMapHeader.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/rmg/CRmgTemplate.h"
#include "../lib/serializer/JsonDeserializer.h"

#include <algorithm>

#include <tbb/global_control.h>

namespace
{
constexpr int MAX_PARALLELISM = 8;
constexpr std::time_t BASE_CREATION_TIME = 1'725'897'600;
constexpr int CREATION_TIME_SPAN_SECONDS = 365 * 24 * 3600;
const std::string FUZZ_TEMPLATE_ID = "2SM2a";
const std::string FUZZ_TEMPLATE_DATA_PATH = "test/rmg/1.json";

std::shared_ptr<CRmgTemplate> loadTemplate()
{
	const JsonNode templateData(JsonPath::builtin(FUZZ_TEMPLATE_DATA_PATH));
	auto result = std::make_shared<CRmgTemplate>();
	result->setId(FUZZ_TEMPLATE_ID);

	JsonDeserializer handler(nullptr, templateData[FUZZ_TEMPLATE_ID]);
	result->serializeJson(handler);

	return result;
}

CRmgTemplate * getTemplate()
{
	static const std::shared_ptr<CRmgTemplate> templateCache = loadTemplate();
	return templateCache.get();
}

}

namespace fuzzing
{
RmgGenerationSpec decodeRmgGenerationSpec(ByteReader & input)
{
	RmgGenerationSpec spec;
	spec.seed = input.readInt();
	if(spec.seed == 0)
		spec.seed = 1;

	spec.singleThread = input.readBool();
	spec.parallelism = input.readInRange(1, MAX_PARALLELISM);
	if(spec.singleThread)
		spec.parallelism = 1;

	spec.creationDateTime = BASE_CREATION_TIME + input.readInRange(0, CREATION_TIME_SPAN_SECONDS);
	return spec;
}

std::unique_ptr<CMap> generateMapWithParallelism(const RmgGenerationSpec & spec, int parallelism)
{
	const int boundedParallelism = std::clamp(parallelism, 1, MAX_PARALLELISM);
	const int maxParallelism = spec.singleThread ? 1 : boundedParallelism;
	tbb::global_control parallelismLimit(tbb::global_control::max_allowed_parallelism, maxParallelism);

	CMapGenOptions options;
	options.setMapTemplate(getTemplate());
	options.setWidth(CMapHeader::MAP_SIZE_SMALL);
	options.setHeight(CMapHeader::MAP_SIZE_SMALL);
	options.setLevels(1);
	options.setHumanOrCpuPlayerCount(2);
	options.setCompOnlyPlayerCount(0);
	options.setPlayerTypeForStandardPlayer(PlayerColor(0), EPlayerType::HUMAN);
	options.setPlayerTypeForStandardPlayer(PlayerColor(1), EPlayerType::AI);
	CMapGenerator generator(options, nullptr, spec.seed);

	auto & mutableConfig = const_cast<CMapGenerator::Config &>(generator.getConfig());
	mutableConfig.singleThread = spec.singleThread;

	return generator.generate(spec.creationDateTime);
}

std::unique_ptr<CMap> generateMap(const RmgGenerationSpec & spec)
{
	return generateMapWithParallelism(spec, spec.parallelism);
}
}
