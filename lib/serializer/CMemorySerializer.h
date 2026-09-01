/*
 * CMemorySerializer.h, part of VCMI engine
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

/// Serializer that stores objects in the dynamic buffer. Allows performing deep object copies.
class DLL_LINKAGE CMemorySerializer
	: public IBinaryReader, public IBinaryWriter
{
	std::vector<std::byte> buffer;

	size_t readPos; //index of the next byte to be read
public:
	BinaryDeserializer iser;
	BinarySerializer oser;

	int read(std::byte * data, unsigned size) override; //throws!
	int write(const std::byte * data, unsigned size) override;

	CMemorySerializer();

	/// Prepares a serializer that reads from a buffer written earlier
	explicit CMemorySerializer(std::vector<std::byte> data);

	/// Hands out everything written so far and leaves this serializer empty
	std::vector<std::byte> extractBuffer();

	template <typename T>
	static std::unique_ptr<T> deepCopy(const T &data, IGameInfoCallback * cb = nullptr)
	{
		CMemorySerializer mem;
		mem.oser & &data;
		mem.iser.cb = cb;

		std::unique_ptr<T> ret;
		mem.iser & ret;
		return ret;
	}

	template <typename T>
	static std::shared_ptr<T> deepCopyShared(const T &data, IGameInfoCallback * cb = nullptr)
	{
		CMemorySerializer mem;
		mem.oser & &data;
		mem.iser.cb = cb;

		std::shared_ptr<T> ret;
		mem.iser & ret;
		return ret;
	}
};
