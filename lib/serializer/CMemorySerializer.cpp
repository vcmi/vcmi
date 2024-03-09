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

VCMI_LIB_NAMESPACE_BEGIN

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
	buffer.resize(oldSize + size);
	std::copy_n(data, size, buffer.data() + oldSize);
	return size;
}

CMemorySerializer::CMemorySerializer(): iser(this), oser(this), readPos(0)
{
	iser.version = ESerializationVersion::CURRENT;
}

VCMI_LIB_NAMESPACE_END
