/*
 * CMemoryBuffer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#include "StdInc.h"
#include "CMemoryBuffer.h"

///CMemoryBuffer
CMemoryBuffer::CMemoryBuffer():
	position(0)
{
	
}

si64 CMemoryBuffer::write(ui8 * data, si64 size)
{
	buffer.reserve(tell()+size);
	
	std::copy(data, data + size, buffer.data() + position);
	position += size;
	
	return size;		
}

si64 CMemoryBuffer::read(ui8 * data, si64 size)
{
	si64 toRead = std::min(getSize() - tell(), size);
	
	std::copy(buffer.data() + position, buffer.data() + position + toRead, data);
	position += toRead;
	
	return toRead;	
}

si64 CMemoryBuffer::seek(si64 position)
{
	this->position = position;
	if (this->position >=getSize())
		this->position = getSize()-1;
	return this->position;
}

si64 CMemoryBuffer::tell()
{
	return position;
}

si64 CMemoryBuffer::skip(si64 delta)
{
	auto old_position = tell();
	
	return seek(old_position + delta) - old_position; 
}

si64 CMemoryBuffer::getSize()
{
	return buffer.size();
}


