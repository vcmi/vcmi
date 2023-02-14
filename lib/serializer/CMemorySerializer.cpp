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

#include "../registerTypes/RegisterTypes.h"

VCMI_LIB_NAMESPACE_BEGIN

int CMemorySerializer::read(void * data, unsigned size)
{
	if(buffer.size() < readPos + size)
		throw std::runtime_error(boost::str(boost::format("Cannot read past the buffer (accessing index %d, while size is %d)!") % (readPos + size - 1) % buffer.size()));

	std::memcpy(data, buffer.data() + readPos, size);
	readPos += size;
	return size;
}

int CMemorySerializer::write(const void * data, unsigned size)
{
	auto oldSize = buffer.size(); //and the pos to write from
	buffer.resize(oldSize + size);
	std::memcpy(buffer.data() + oldSize, data, size);
	return size;
}

CMemorySerializer::CMemorySerializer(): iser(this), oser(this), readPos(0)
{
	registerTypes(iser);
	registerTypes(oser);
	iser.fileVersion = SERIALIZATION_VERSION;
}

VCMI_LIB_NAMESPACE_END
