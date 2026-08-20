/*
 * CBinaryCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BinarySerializer.h"
#include "BinaryDeserializer.h"

#include <vector>

/// Magic values used to identify binary metadata cache files.
namespace BinaryCache
{
	inline constexpr const char * MAP_MAGIC = "VCML";
	inline constexpr const char * CAMPAIGN_MAGIC = "VCCL";
}

/// Writes map/campaign metadata into an in-memory binary cache buffer.
/// The produced stream starts with a 4-byte magic followed by the serialization version.
class DLL_LINKAGE CBinaryCacheWriter final : public IBinaryWriter
{
	std::vector<std::byte> buffer;
	BinarySerializer serializer;

	int write(const std::byte * data, unsigned size) final;

public:
	explicit CBinaryCacheWriter(const char * magic);

	BinarySerializer & getSerializer()
	{
		return serializer;
	}

	const std::vector<std::byte> & getBuffer() const
	{
		return buffer;
	}
};

/// Reads map/campaign metadata from an in-memory binary cache buffer.
/// Validates the magic and serialization version on construction.
class DLL_LINKAGE CBinaryCacheReader final : public IBinaryReader
{
	const std::byte * buffer;
	size_t size;
	size_t position;
	BinaryDeserializer deserializer;

	int read(std::byte * data, unsigned size) final;

public:
	CBinaryCacheReader(const std::byte * buffer, size_t size, const char * magic);

	BinaryDeserializer & getDeserializer()
	{
		return deserializer;
	}
};
