/*
 * CBinaryCache.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBinaryCache.h"

#include <algorithm>
#include <cstring>

CBinaryCacheWriter::CBinaryCacheWriter(const char * magic)
	: serializer(this)
{
	write(reinterpret_cast<const std::byte *>(magic), 4);

	ESerializationVersion version = ESerializationVersion::CURRENT;
	write(reinterpret_cast<const std::byte *>(&version), sizeof(version));
}

int CBinaryCacheWriter::write(const std::byte * data, unsigned size)
{
	buffer.insert(buffer.end(), data, data + size);
	return size;
}

CBinaryCacheReader::CBinaryCacheReader(const std::byte * buffer, size_t size, const char * magic)
	: buffer(buffer)
	, size(size)
	, position(0)
	, deserializer(this)
{
	if(size < 8)
		throw std::runtime_error("Invalid binary cache: file is too short");

	if(std::memcmp(buffer, magic, 4) != 0)
		throw std::runtime_error("Invalid binary cache: wrong magic");

	position = 4;

	ESerializationVersion version;
	std::memcpy(&version, buffer + position, sizeof(version));
	position += sizeof(version);

	if(version < ESerializationVersion::MINIMAL)
		throw std::runtime_error("Invalid binary cache: too old format version");
	if(version > ESerializationVersion::CURRENT)
		throw std::runtime_error("Invalid binary cache: too new format version");

	deserializer.version = version;
}

int CBinaryCacheReader::read(std::byte * data, unsigned size)
{
	if(position + size > this->size)
		throw std::runtime_error("Invalid binary cache: unexpected end of file");

	std::copy_n(buffer + position, size, data);
	position += size;
	return size;
}
