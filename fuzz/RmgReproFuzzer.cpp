/*
 * RmgReproFuzzer.cpp, part of VCMI engine
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
#include "../lib/mapObjects/ObjectTemplate.h"
#include "../lib/serializer/BinarySerializer.h"

#include <boost/algorithm/string/case_conv.hpp>

#include <cstddef>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

namespace
{
bool strictThreadInvariantMode()
{
	static const bool enabled = []()
	{
		if(const char * value = std::getenv("VCMI_FUZZ_CHECK_THREAD_INVARIANCE"))
		{
			const std::string lowered = boost::algorithm::to_lower_copy(std::string(value));
			return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
		}
		return false;
	}();

	return enabled;
}

class VectorBinaryWriter final : public IBinaryWriter
{
public:
	int write(const std::byte * data, unsigned size) override
	{
		buffer.insert(buffer.end(), data, data + size);
		return static_cast<int>(size);
	}

	[[nodiscard]] const std::vector<std::byte> & bytes() const
	{
		return buffer;
	}

private:
	std::vector<std::byte> buffer;
};

std::vector<std::byte> serializeMapState(const CMap & map)
{
	VectorBinaryWriter writer;
	BinarySerializer serializer(&writer);

	serializer & map.width;
	serializer & map.height;
	serializer & map.creationDateTime;
	serializer & map.difficulty;
	serializer & map.allowedArtifact;
	serializer & map.allowedSpells;

	for(int z = 0; z < map.levels(); ++z)
	{
		for(int y = 0; y < map.height; ++y)
		{
			for(int x = 0; x < map.width; ++x)
				serializer & map.getTile(int3(x, y, z));
		}
	}

	const auto objectsCount = map.objects.size();
	serializer & objectsCount;
	for(const auto & object : map.objects)
	{
		const auto * objectPtr = object.get();
		const bool hasObject = objectPtr != nullptr;
		serializer & hasObject;
		if(!hasObject)
			continue;

		serializer & objectPtr->id;
		serializer & objectPtr->ID;
		const int subId = objectPtr->subID.getNum();
		serializer & subId;
		serializer & objectPtr->tempOwner;
		serializer & objectPtr->pos.x;
		serializer & objectPtr->pos.y;
		serializer & objectPtr->pos.z;
		serializer & objectPtr->blockVisit;
		serializer & objectPtr->removable;
		serializer & objectPtr->rmgValue;
		serializer & objectPtr->instanceName;

		int heroType = -1;
		if(const auto * hero = dynamic_cast<const CGHeroInstance *>(objectPtr))
		{
			const auto heroTypeId = hero->getHeroTypeID();
			heroType = heroTypeId.hasValue() ? heroTypeId.getNum() : -1;
		}
		serializer & heroType;

		int artifactType = -1;
		if(objectPtr->ID == Obj::ARTIFACT)
		{
			const auto * artifact = dynamic_cast<const CGArtifact *>(objectPtr);
			if(artifact)
				artifactType = artifact->getArtifactType().getNum();
		}
		serializer & artifactType;
	}

	return writer.bytes();
}

std::optional<std::vector<std::byte>> generateSerializedMapState(const fuzzing::RmgGenerationSpec & spec, int parallelism)
{
	try
	{
		auto map = fuzzing::generateMapWithParallelism(spec, parallelism);
		return serializeMapState(*map);
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

	const auto baselineState = generateSerializedMapState(spec, spec.parallelism);
	if(!baselineState)
		return 0;

	const auto replayState = generateSerializedMapState(spec, spec.parallelism);
	if(!replayState)
		return 0;

	if(*baselineState != *replayState)
		std::abort();

	if(!spec.singleThread && strictThreadInvariantMode())
	{
		const int alternateParallelism = spec.parallelism == 1 ? 4 : 1;
		const auto alternateState = generateSerializedMapState(spec, alternateParallelism);
		if(alternateState && *baselineState != *alternateState)
			std::abort();
	}

	return 0;
}
