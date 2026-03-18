/*
 * RmgValidityFuzzer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "FuzzEnvironment.h"
#include "RmgFuzzSpec.h"

#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/mapObjects/MiscObjects.h"

#include <cstdlib>
#include <optional>

namespace
{
size_t countMapIssues(const CMap & map)
{
	size_t issueCount = 0;

	for(const auto & objectPtr : map.objects)
	{
		if(!objectPtr)
			continue;

		const auto * object = objectPtr.get();
		const auto owner = object->getOwner();

		if(owner == PlayerColor::UNFLAGGABLE && object->asOwnable())
			++issueCount;

		if(owner != PlayerColor::NEUTRAL
			&& owner.getNum() < map.players.size()
			&& !map.players[owner.getNum()].canAnyonePlay())
		{
			++issueCount;
		}

		if(const auto * hero = dynamic_cast<const CGHeroInstance *>(object))
		{
			const auto heroType = hero->getHeroTypeID();
			if(heroType.hasValue() && map.allowedHeroes.count(heroType) == 0)
				++issueCount;
		}

		if(const auto * artifact = dynamic_cast<const CGArtifact *>(object))
		{
			if(artifact->ID == Obj::ARTIFACT
				&& map.allowedArtifact.count(artifact->getArtifactType()) == 0)
				++issueCount;
		}
	}

	return issueCount;
}

std::optional<size_t> generateAndValidate(
	const fuzzing::RmgGenerationSpec & spec,
	int parallelism)
{
	try
	{
		auto map = fuzzing::generateMapWithParallelism(spec, parallelism);
		return countMapIssues(*map);
	}
	catch(const std::exception &)
	{
		return std::nullopt;
	}
}
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
	fuzzing::initializeEngine();

	fuzzing::ByteReader input(data, size);
	const auto spec = fuzzing::decodeRmgGenerationSpec(input);

	const auto primaryIssues = generateAndValidate(spec, spec.parallelism);
	if(primaryIssues && *primaryIssues != 0)
		std::abort();

	if(!spec.singleThread)
	{
		const int alternateParallelism = spec.parallelism == 1 ? 4 : 1;
		const auto alternateIssues = generateAndValidate(spec, alternateParallelism);
		if(alternateIssues && *alternateIssues != 0)
			std::abort();
	}

	return 0;
}
