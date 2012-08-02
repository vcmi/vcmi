#include "StdInc.h"
#include "CMemoryStream.h"

CMemoryStream::CMemoryStream(const ui8 * data, si64 size, bool freeData /*= false*/):
    data(data),
    size(size),
    position(0),
    freeData(freeData)
{
}

CMemoryStream::~CMemoryStream()
{
	if(freeData)
	{
		delete[] data;
	}
}

si64 CMemoryStream::read(ui8 * data, si64 size)
{
	std::copy(this->data + position, this->data + position + size, data);
	position += size;
	return size;
}

si64 CMemoryStream::seek(si64 position)
{
	si64 diff = this->position;
	this->position = position;
	return position - diff;
}

si64 CMemoryStream::tell()
{
	return this->position;
}

si64 CMemoryStream::skip(si64 delta)
{
	this->position += delta;
	return delta;
}

si64 CMemoryStream::getSize()
{
	return size;
}
