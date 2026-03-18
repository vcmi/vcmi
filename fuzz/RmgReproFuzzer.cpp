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

#include <cstdlib>
#include <optional>
#include <string>
#include <type_traits>

namespace
{
class MapFingerprintBuilder
{
public:
	template<typename T>
	void addIntegral(T value)
	{
		static_assert(std::is_integral<T>::value, "Integral value expected");
		using Unsigned = typename std::make_unsigned<T>::type;
		addUnsigned(static_cast<uint64_t>(static_cast<Unsigned>(value)));
	}

	void addBool(bool value)
	{
		addIntegral(value ? 1u : 0u);
	}

	void addString(const std::string & value)
	{
		addIntegral(static_cast<uint64_t>(value.size()));
		for(unsigned char symbol : value)
			mixByte(symbol);
	}

	[[nodiscard]] uint64_t finish() const
	{
		return state;
	}

private:
	void addUnsigned(uint64_t value)
	{
		for(int i = 0; i < 8; ++i)
			mixByte(static_cast<uint8_t>((value >> (i * 8)) & 0xffu));
	}

	void mixByte(uint8_t byte)
	{
		state ^= byte;
		state *= 1099511628211ULL;
	}

	uint64_t state = 1469598103934665603ULL;
};

void hashTile(MapFingerprintBuilder & hash, const TerrainTile & tile)
{
	hash.addIntegral(tile.terrainType.getNum());
	hash.addIntegral(tile.riverType.getNum());
	hash.addIntegral(tile.roadType.getNum());
	hash.addIntegral(tile.terView);
	hash.addIntegral(tile.riverDir);
	hash.addIntegral(tile.roadDir);
	hash.addIntegral(tile.extTileFlags);

	hash.addIntegral(static_cast<uint64_t>(tile.visitableObjects.size()));
	for(const auto visitableId : tile.visitableObjects)
		hash.addIntegral(visitableId.getNum());

	hash.addIntegral(static_cast<uint64_t>(tile.blockingObjects.size()));
	for(const auto blockingId : tile.blockingObjects)
		hash.addIntegral(blockingId.getNum());
}

void hashObject(MapFingerprintBuilder & hash, const CGObjectInstance * object)
{
	if(object == nullptr)
	{
		hash.addBool(false);
		return;
	}

	hash.addBool(true);
	hash.addIntegral(object->id.getNum());
	hash.addIntegral(object->ID.getNum());
	hash.addIntegral(object->subID.getNum());
	hash.addIntegral(object->tempOwner.getNum());
	hash.addIntegral(object->pos.x);
	hash.addIntegral(object->pos.y);
	hash.addIntegral(object->pos.z);
	hash.addBool(object->blockVisit);
	hash.addBool(object->removable);
	hash.addIntegral(object->rmgValue);
	hash.addString(object->instanceName);

	if(const auto * hero = dynamic_cast<const CGHeroInstance *>(object))
	{
		const auto heroType = hero->getHeroTypeID();
		hash.addIntegral(heroType.hasValue() ? heroType.getNum() : -1);
	}
	else
	{
		hash.addIntegral(-1);
	}

	if(const auto * artifact = dynamic_cast<const CGArtifact *>(object))
		hash.addIntegral(artifact->getArtifactType().getNum());
	else
		hash.addIntegral(-1);
}

uint64_t mapFingerprint(const CMap & map)
{
	MapFingerprintBuilder hash;
	hash.addIntegral(map.width);
	hash.addIntegral(map.height);
	hash.addIntegral(map.levels());
	hash.addIntegral(static_cast<int64_t>(map.creationDateTime));
	hash.addIntegral(static_cast<int>(map.difficulty));

	hash.addIntegral(static_cast<uint64_t>(map.allowedArtifact.size()));
	for(const auto artifact : map.allowedArtifact)
		hash.addIntegral(artifact.getNum());

	hash.addIntegral(static_cast<uint64_t>(map.allowedSpells.size()));
	for(const auto spell : map.allowedSpells)
		hash.addIntegral(spell.getNum());

	for(int z = 0; z < map.levels(); ++z)
	{
		for(int y = 0; y < map.height; ++y)
		{
			for(int x = 0; x < map.width; ++x)
				hashTile(hash, map.getTile(int3(x, y, z)));
		}
	}

	hash.addIntegral(static_cast<uint64_t>(map.objects.size()));
	for(const auto & object : map.objects)
		hashObject(hash, object.get());

	return hash.finish();
}

std::optional<uint64_t> generateFingerprint(const fuzzing::RmgGenerationSpec & spec, int parallelism)
{
	try
	{
		auto map = fuzzing::generateMapWithParallelism(spec, parallelism);
		return mapFingerprint(*map);
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

	const auto baseline = generateFingerprint(spec, spec.parallelism);
	if(!baseline)
		return 0;

	const auto replay = generateFingerprint(spec, spec.parallelism);
	if(!replay)
		return 0;

	if(*baseline != *replay)
		std::abort();

	if(!spec.singleThread)
	{
		const int alternateParallelism = spec.parallelism == 1 ? 4 : 1;
		const auto alternate = generateFingerprint(spec, alternateParallelism);
		if(alternate && *baseline != *alternate)
			std::abort();
	}

	return 0;
}
