/*
 * CBinaryReader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBinaryReader.h"

#include "CInputStream.h"
#include "../TextOperations.h"

VCMI_LIB_NAMESPACE_BEGIN

#ifdef VCMI_ENDIAN_BIG
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

CBinaryReader::CBinaryReader(CInputStream * stream) : stream(stream)
{
}

CInputStream * CBinaryReader::getStream()
{
	return stream;
}

void CBinaryReader::setStream(CInputStream * stream)
{
	this->stream = stream;
}

si64 CBinaryReader::read(ui8 * data, si64 size)
{
	si64 bytesRead = stream->read(data, size);
	if(bytesRead != size)
	{
		throw std::runtime_error(getEndOfStreamExceptionMsg((long)size));
	}
	return bytesRead;
}

template <typename CData>
CData CBinaryReader::readInteger()
{
	CData val;
	stream->read(reinterpret_cast<unsigned char *>(&val), sizeof(val));
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

std::string CBinaryReader::readString()
{
	unsigned int len = readUInt32();
	assert(len <= 500000); //not too long
	if (len > 0)
	{
		std::string ret;
		ret.resize(len);
		read(reinterpret_cast<ui8*>(&ret[0]), len);
		//FIXME: any need to move this into separate "read localized string" method?
		if (Unicode::isValidASCII(ret))
			return ret;
		return Unicode::toUnicode(ret);
	}
	return "";

}

void CBinaryReader::skip(int count)
{
	stream->skip(count);
}

std::string CBinaryReader::getEndOfStreamExceptionMsg(long bytesToRead) const
{
	std::stringstream ss;
	ss << "The end of the stream was reached unexpectedly. The stream has a length of " << stream->getSize() << " and the current reading position is "
				<< stream->tell() << ". The client wanted to read " << bytesToRead << " bytes.";

	return ss.str();
}

VCMI_LIB_NAMESPACE_END
