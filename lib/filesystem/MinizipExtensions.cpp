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

VCMI_LIB_NAMESPACE_BEGIN

template<class Stream>
inline uLong streamRead(voidpf opaque, voidpf stream, void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<Stream *>(stream);

	return static_cast<uLong>(actualStream->read(static_cast<ui8 *>(buf), size));
}

template<class Stream>
inline ZPOS64_T streamTell(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<Stream *>(stream);
	return actualStream->tell();
}

template<class Stream>
inline long streamSeek(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<Stream *>(stream);

	long ret = 0;
	switch(origin)
	{
		case ZLIB_FILEFUNC_SEEK_CUR:
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
		case ZLIB_FILEFUNC_SEEK_SET:
			if(actualStream->seek(offset) != offset)
				ret = -1;
			break;
		default:
			ret = -1;
	}
	if(ret == -1)
		logGlobal->error("Stream seek failed");
	return 0;
}

template<class Stream>
inline int streamProxyClose(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	auto * actualStream = static_cast<Stream *>(stream);

	logGlobal->trace("Proxy stream closed");

	actualStream->seek(0);

    return 0;
}

///CDefaultIOApi
#define GETFILE static_cast<std::FILE*>(filePtr)

#ifdef VCMI_WINDOWS
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#include <cwchar>
	#define CHAR_LITERAL(s) L##s
	using CharType = wchar_t;
#else
	#define CHAR_LITERAL(s) s
	using CharType = char;
#endif

static inline FILE* do_open(const CharType* name, const CharType* mode)
{
	#ifdef VCMI_WINDOWS
		return _wfopen(name, mode);
	#else
		return std::fopen(name, mode);
	#endif
}

static voidpf ZCALLBACK MinizipOpenFunc(voidpf opaque, const void* filename, int mode)
{
	const CharType* mode_fopen = [mode]() -> const CharType*
	{
		if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
			return CHAR_LITERAL("rb");
		else if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
			return CHAR_LITERAL("r+b");
		else if (mode & ZLIB_FILEFUNC_MODE_CREATE)
			return CHAR_LITERAL("wb");
		return nullptr;
	}();

	if (filename != nullptr && mode_fopen != nullptr)
		return do_open(static_cast<const CharType*>(filename), mode_fopen);
	else
		return nullptr;
}

zlib_filefunc64_def CDefaultIOApi::getApiStructure()
{
	static zlib_filefunc64_def MinizipFilefunc;
	static bool initialized = false;
	if (!initialized)
	{
		fill_fopen64_filefunc(&MinizipFilefunc);
		MinizipFilefunc.zopen64_file = &MinizipOpenFunc;
		initialized = true;
	}
	return MinizipFilefunc;
}

///CProxyIOApi
CProxyIOApi::CProxyIOApi(CInputOutputStream * buffer):
	data(buffer)
{

}
//must be instantiated in .cpp file for access to complete types of all member fields
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

	return (static_cast<CProxyIOApi *>(opaque))->openFile(path, mode);
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
	return static_cast<uLong>(actualStream->write(static_cast<const ui8 *>(buf), size));
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

//must be instantiated in .cpp file for access to complete types of all member fields
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

	return (static_cast<CProxyROIOApi *>(opaque))->openFile(path, mode);
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
