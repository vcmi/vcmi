/*
 * MinizipExtensions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MinizipExtensions.h"

#include "CMemoryBuffer.h"
#include "FileStream.h"

VCMI_LIB_NAMESPACE_BEGIN

template <class _Stream> inline uLong streamRead(voidpf opaque, voidpf stream, void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<_Stream *>(stream);

	return (uLong)actualStream->read((ui8 *)buf, (uLong)size);
}

template <class _Stream> inline ZPOS64_T streamTell(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<_Stream *>(stream);
    return actualStream->tell();
}

template <class _Stream> inline long streamSeek(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<_Stream *>(stream);

    long ret = 0;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        if(actualStream->skip(offset) != offset)
			ret = -1;
        break;
    case ZLIB_FILEFUNC_SEEK_END:
    	{
    		const si64 pos = actualStream->getSize() - offset;
    		if(actualStream->seek(pos) != pos)
				ret = -1;
    	}
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
		if(actualStream->seek(offset) != offset)
			ret = -1;
        break;
    default: ret = -1;
    }
    if(ret == -1)
		logGlobal->error("Stream seek failed");
    return ret;
}

template <class _Stream> inline int streamProxyClose(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<_Stream *>(stream);

	logGlobal->trace("Proxy stream closed");

	actualStream->seek(0);

    return 0;
}

///CDefaultIOApi
CDefaultIOApi::CDefaultIOApi() = default;

CDefaultIOApi::~CDefaultIOApi() = default;

zlib_filefunc64_def CDefaultIOApi::getApiStructure()
{
	return * FileStream::GetMinizipFilefunc();
}

///CProxyIOApi
CProxyIOApi::CProxyIOApi(CInputOutputStream * buffer):
	data(buffer)
{

}

CProxyIOApi::~CProxyIOApi() = default;

zlib_filefunc64_def CProxyIOApi::getApiStructure()
{
	zlib_filefunc64_def api;
	api.opaque = this;
	api.zopen64_file = &openFileProxy;
	api.zread_file = &readFileProxy;
	api.zwrite_file = &writeFileProxy;
	api.ztell64_file = &tellFileProxy;
	api.zseek64_file = &seekFileProxy;
	api.zclose_file = &closeFileProxy;
	api.zerror_file = &errorFileProxy;

	return api;
}

voidpf ZCALLBACK CProxyIOApi::openFileProxy(voidpf opaque, const void * filename, int mode)
{
	assert(opaque != nullptr);

	boost::filesystem::path path;

	if(filename != nullptr)
		path =  static_cast<const boost::filesystem::path::value_type *>(filename);

	return ((CProxyIOApi *)opaque)->openFile(path, mode);
}

uLong ZCALLBACK CProxyIOApi::readFileProxy(voidpf opaque, voidpf stream, void * buf, uLong size)
{
	return streamRead<CInputOutputStream>(opaque, stream, buf, size);
}

uLong ZCALLBACK CProxyIOApi::writeFileProxy(voidpf opaque, voidpf stream, const void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<CInputOutputStream *>(stream);
    return (uLong)actualStream->write((const ui8 *)buf, size);
}

ZPOS64_T ZCALLBACK CProxyIOApi::tellFileProxy(voidpf opaque, voidpf stream)
{
	return streamTell<CInputOutputStream>(opaque, stream);
}

long ZCALLBACK CProxyIOApi::seekFileProxy(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	return streamSeek<CInputOutputStream>(opaque, stream, offset, origin);
}

int ZCALLBACK CProxyIOApi::closeFileProxy(voidpf opaque, voidpf stream)
{
	return streamProxyClose<CInputOutputStream>(opaque, stream);
}

int ZCALLBACK CProxyIOApi::errorFileProxy(voidpf opaque, voidpf stream)
{
    return 0;
}

CInputOutputStream * CProxyIOApi::openFile(const boost::filesystem::path & filename, int mode)
{
	logGlobal->trace("CProxyIOApi: stream opened for %s with mode %d", filename.string(), mode);

	data->seek(0);
	return data;
}

///CProxyROIOApi
CProxyROIOApi::CProxyROIOApi(CInputStream * buffer):
	data(buffer)
{

}

CProxyROIOApi::~CProxyROIOApi() = default;

zlib_filefunc64_def CProxyROIOApi::getApiStructure()
{
	zlib_filefunc64_def api;
	api.opaque = this;
	api.zopen64_file = &openFileProxy;
	api.zread_file = &readFileProxy;
	api.zwrite_file = &writeFileProxy;
	api.ztell64_file = &tellFileProxy;
	api.zseek64_file = &seekFileProxy;
	api.zclose_file = &closeFileProxy;
	api.zerror_file = &errorFileProxy;

	return api;
}

CInputStream * CProxyROIOApi::openFile(const boost::filesystem::path& filename, int mode)
{
	logGlobal->trace("CProxyROIOApi: stream opened for %s with mode %d", filename.string(), mode);

	data->seek(0);
	return data;
}

voidpf ZCALLBACK CProxyROIOApi::openFileProxy(voidpf opaque, const void* filename, int mode)
{
	assert(opaque != nullptr);

	boost::filesystem::path path;

	if(filename != nullptr)
		path =  static_cast<const boost::filesystem::path::value_type *>(filename);

	return ((CProxyROIOApi *)opaque)->openFile(path, mode);
}

uLong ZCALLBACK CProxyROIOApi::readFileProxy(voidpf opaque, voidpf stream, void * buf, uLong size)
{
	return streamRead<CInputStream>(opaque, stream, buf, size);
}

uLong ZCALLBACK CProxyROIOApi::writeFileProxy(voidpf opaque, voidpf stream, const void* buf, uLong size)
{
	logGlobal->error("Attempt to write to read-only stream");
	return 0;
}

ZPOS64_T ZCALLBACK CProxyROIOApi::tellFileProxy(voidpf opaque, voidpf stream)
{
	return streamTell<CInputStream>(opaque, stream);
}

long ZCALLBACK CProxyROIOApi::seekFileProxy(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	return streamSeek<CInputStream>(opaque, stream, offset, origin);
}

int ZCALLBACK CProxyROIOApi::closeFileProxy(voidpf opaque, voidpf stream)
{
	return streamProxyClose<CInputStream>(opaque, stream);
}

int ZCALLBACK CProxyROIOApi::errorFileProxy(voidpf opaque, voidpf stream)
{
	return 0;
}

VCMI_LIB_NAMESPACE_END
