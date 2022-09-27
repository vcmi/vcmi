/*
 * CMemoryStream.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMemoryStream.h"

VCMI_LIB_NAMESPACE_BEGIN

CMemoryStream::CMemoryStream(const ui8 * data, si64 size) :
	data(data), size(size), position(0)
{

}

si64 CMemoryStream::read(ui8 * data, si64 size)
{
	si64 toRead = std::min(this->size - tell(), size);
	std::copy(this->data + position, this->data + position + toRead, data);
	position += size;
	return toRead;
}

si64 CMemoryStream::seek(si64 position)
{
	si64 origin = tell();
	this->position = std::min(position, size);
	return tell() - origin;
}

si64 CMemoryStream::tell()
{
	return this->position;
}

si64 CMemoryStream::skip(si64 delta)
{
	si64 origin = tell();
	this->position += std::min(size - origin, delta);
	return tell() - origin;
}

si64 CMemoryStream::getSize()
{
	return size;
}

VCMI_LIB_NAMESPACE_END
