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

#include "../lib/callback/EditorCallback.h"
#include "../lib/mapping/CMapHeader.h"
#include "../lib/rmg/CMapGenerator.h"
#include "../lib/rmg/CRmgTemplate.h"

#include <algorithm>

#include <tbb/global_control.h>

namespace
{
constexpr std::time_t BASE_CREATION_TIME = 1'725'897'600;
constexpr int CREATION_TIME_SPAN_SECONDS = 365 * 24 * 3600;

const CRmgTemplate * pickDeterministicTemplate(const CMapGenOptions & options)
{
	auto candidates = options.getPossibleTemplates();
	if(candidates.empty())
		throw std::runtime_error("No RMG templates available for fuzzing.");

	return *std::min_element(candidates.begin(), candidates.end(), [](const CRmgTemplate * lhs, const CRmgTemplate * rhs)
	{
		return lhs->getId() < rhs->getId();
	});
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
	spec.parallelism = std::max(1, input.readInt());
	if(spec.singleThread)
		spec.parallelism = 1;

	spec.creationDateTime = BASE_CREATION_TIME + input.readInRange(0, CREATION_TIME_SPAN_SECONDS);
	return spec;
}

std::unique_ptr<CMap> generateMapWithParallelism(const RmgGenerationSpec & spec, int parallelism)
{
	const int maxParallelism = spec.singleThread ? 1 : std::max(1, parallelism);
	tbb::global_control parallelismLimit(tbb::global_control::max_allowed_parallelism, maxParallelism);

	// RMG object creation expects a non-null callback.
	CMap callbackMap(nullptr);
	EditorCallback callback(&callbackMap);

	CMapGenOptions options;
	options.setWidth(CMapHeader::MAP_SIZE_SMALL);
	options.setHeight(CMapHeader::MAP_SIZE_SMALL);
	options.setLevels(1);
	options.setHumanOrCpuPlayerCount(2);
	options.setCompOnlyPlayerCount(0);
	options.setPlayerTypeForStandardPlayer(PlayerColor(0), EPlayerType::HUMAN);
	options.setPlayerTypeForStandardPlayer(PlayerColor(1), EPlayerType::AI);
	options.setMapTemplate(pickDeterministicTemplate(options));
	CMapGenerator generator(options, &callback, spec.seed);

	generator.setSingleThread(spec.singleThread);

	return generator.generate(spec.creationDateTime);
}

std::unique_ptr<CMap> generateMap(const RmgGenerationSpec & spec)
{
	return generateMapWithParallelism(spec, spec.parallelism);
}
}
