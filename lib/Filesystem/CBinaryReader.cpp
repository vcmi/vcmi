#include "StdInc.h"
#include "CBinaryReader.h"
#include <SDL_endian.h>
#include "CInputStream.h"

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

ui8 CBinaryReader::readUInt8()
{
	ui8 val;
	si64 b = stream->read(&val, 1);
	if(b < 1)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(1));
	}

	return val;
}

si8 CBinaryReader::readInt8()
{
	si8 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 1);
	if(b < 1)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(1));
	}

	return val;
}

ui16 CBinaryReader::readUInt16()
{
	ui16 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 2);
	if(b < 2)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(2));
	}

	return SDL_SwapLE16(val);
}

si16 CBinaryReader::readInt16()
{
	si16 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 2);
	if(b < 2)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(2));
	}

	return SDL_SwapLE16(val);
}

ui32 CBinaryReader::readUInt32()
{
	ui32 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 4);
	if(b < 4)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(4));
	}

	return SDL_SwapLE32(val);
}

si32 CBinaryReader::readInt32()
{
	si32 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 4);
	if(b < 4)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(4));
	}

	return SDL_SwapLE32(val);
}

ui64 CBinaryReader::readUInt64()
{
	ui64 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 8);
	if(b < 8)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(8));
	}

	return SDL_SwapLE64(val);
}

si64 CBinaryReader::readInt64()
{
	si64 val;
	si64 b = stream->read(reinterpret_cast<ui8 *>(&val), 8);
	if(b < 8)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg(8));
	}

	return SDL_SwapLE64(val);
}

std::string CBinaryReader::getEndOfStreamExceptionMsg(long bytesToRead) const
{
	std::stringstream ss;
	ss << "The end of the stream was reached unexpectedly. The stream has a length of " << stream->getSize() << " and the current reading position is "
				<< stream->tell() << ". The client wanted to read " << bytesToRead << " bytes.";

	return ss.str();
}
