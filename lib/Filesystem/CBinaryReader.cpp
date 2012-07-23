#include "StdInc.h"
#include "CBinaryReader.h"
#include <SDL_endian.h>
#include "CInputStream.h"


#if SDL_BYTEORDER == SDL_BIG_ENDIAN
template <typename CData>
CData readLE(CData data)
{
	auto dataPtr = (char*)&data;
	std::reverse(dataPtr, dataPtr + sizeof(data));
	return data;
}

#else

template <typename CData>
CData readLE(CData data)
{
	return data;
}
#endif

CBinaryReader::CBinaryReader() : stream(nullptr)
{

}

CBinaryReader::CBinaryReader(CInputStream & stream) : stream(&stream)
{

}

CInputStream * CBinaryReader::getStream()
{
	return stream;
}

void CBinaryReader::setStream(CInputStream & stream)
{
	this->stream = &stream;
}

si64 CBinaryReader::read(ui8 * data, si64 size)
{
	return stream->read(data, size);
}

template <typename CData>
CData CBinaryReader::readInteger()
{
	CData val;
	si64 b = stream->read(reinterpret_cast<unsigned char *>(&val), sizeof(val));
	if(b < sizeof(val))
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(sizeof(val)));
	}

	return readLE(val);
}

//FIXME: any way to do this without macro?
#define INSTANTIATE(datatype, methodname) \
datatype CBinaryReader::methodname() \
{ return readInteger<datatype>(); }

// While it is certanly possible to leave only template method
// but typing template parameter every time can be annoying
// and templates parameters can't be resolved by return type
INSTANTIATE(ui8, readUInt8)
INSTANTIATE(si8, readInt8)
INSTANTIATE(ui16, readUInt16)
INSTANTIATE(si16, readInt16)
INSTANTIATE(ui32, readUInt32)
INSTANTIATE(si32, readInt32)
INSTANTIATE(ui64, readUInt64)
INSTANTIATE(si64, readInt64)

#undef INSTANTIATE

std::string CBinaryReader::getEndOfStreamExceptionMsg(long bytesToRead) const
{
	std::stringstream ss;
	ss << "The end of the stream was reached unexpectedly. The stream has a length of " << stream->getSize() << " and the current reading position is "
				<< stream->tell() << ". The client wanted to read " << bytesToRead << " bytes.";

	return ss.str();
}
