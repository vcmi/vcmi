/*
 * CMemorySerializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMemorySerializer.h"

int CMemorySerializer::read(std::byte * data, unsigned size)
{
	if(buffer.size() < readPos + size)
		throw std::runtime_error(boost::str(boost::format("Cannot read past the buffer (accessing index %d, while size is %d)!") % (readPos + size - 1) % buffer.size()));

	std::copy_n(buffer.data() + readPos, size, data);
	readPos += size;
	return size;
}

int CMemorySerializer::write(const std::byte * data, unsigned size)
{
	auto oldSize = buffer.size(); //and the pos to write from
	auto newSize = oldSize + size;

	// write() is called once per primitive (often just 1-2 bytes at a time), so this runs
	// a huge number of times while serializing a large object graph (e.g. a gamestate snapshot).
	// Growing capacity geometrically here avoids relying on vector::resize() to do it for us,
	// which is not guaranteed to grow with an amortized strategy the way push_back() does.
	if(newSize > buffer.capacity())
		buffer.reserve(std::max<size_t>(newSize, buffer.capacity() * 2));

	buffer.resize(newSize);
	std::copy_n(data, size, buffer.data() + oldSize);
	return size;
}

CMemorySerializer::CMemorySerializer(): CMemorySerializer(std::vector<std::byte>())
{
}

CMemorySerializer::CMemorySerializer(std::vector<std::byte> data): buffer(std::move(data)), readPos(0), iser(this), oser(this)
{
	iser.version = ESerializationVersion::CURRENT;
}

std::vector<std::byte> CMemorySerializer::extractBuffer()
{
	std::vector<std::byte> result = std::move(buffer);
	buffer.clear();
	readPos = 0;
	return result;
}
