/*
 * MinizipExtensions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#ifdef USE_SYSTEM_MINIZIP
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <minizip/ioapi.h>
#else
#include "../minizip/unzip.h"
#include "../minizip/zip.h"
#include "../minizip/ioapi.h"
#endif

VCMI_LIB_NAMESPACE_BEGIN

class CInputStream;
class CInputOutputStream;
class CMemoryBuffer;

class DLL_LINKAGE CIOApi
{
public:
	virtual ~CIOApi(){};

	virtual zlib_filefunc64_def getApiStructure() = 0;
};

///redirects back to minizip ioapi
//todo: replace with Virtual FileSystem interface
class DLL_LINKAGE CDefaultIOApi: public CIOApi
{
public:
	CDefaultIOApi();
	~CDefaultIOApi();

	zlib_filefunc64_def getApiStructure() override;
};

///redirects all file IO to single stream
class DLL_LINKAGE CProxyIOApi: public CIOApi
{
public:
	CProxyIOApi(CInputOutputStream * buffer);
	~CProxyIOApi();

	zlib_filefunc64_def getApiStructure() override;
private:
	CInputOutputStream * openFile(const boost::filesystem::path & filename, int mode);

	CInputOutputStream * data;

	static voidpf ZCALLBACK openFileProxy(voidpf opaque, const void * filename, int mode);
	static uLong ZCALLBACK readFileProxy(voidpf opaque, voidpf stream, void * buf, uLong size);
	static uLong ZCALLBACK writeFileProxy(voidpf opaque, voidpf stream, const void * buf, uLong size);
	static ZPOS64_T ZCALLBACK tellFileProxy(voidpf opaque, voidpf stream);
	static long ZCALLBACK seekFileProxy(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin);
	static int ZCALLBACK closeFileProxy(voidpf opaque, voidpf stream);
	static int ZCALLBACK errorFileProxy(voidpf opaque, voidpf stream);
};

///redirects all file IO to single stream read-only
class DLL_LINKAGE CProxyROIOApi: public CIOApi
{
public:
	CProxyROIOApi(CInputStream * buffer);
	~CProxyROIOApi();

	zlib_filefunc64_def getApiStructure() override;
private:
	CInputStream * openFile(const boost::filesystem::path & filename, int mode);

	CInputStream * data;

	static voidpf ZCALLBACK openFileProxy(voidpf opaque, const void * filename, int mode);
	static uLong ZCALLBACK readFileProxy(voidpf opaque, voidpf stream, void * buf, uLong size);
	static uLong ZCALLBACK writeFileProxy(voidpf opaque, voidpf stream, const void * buf, uLong size);
	static ZPOS64_T ZCALLBACK tellFileProxy(voidpf opaque, voidpf stream);
	static long ZCALLBACK seekFileProxy(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin);
	static int ZCALLBACK closeFileProxy(voidpf opaque, voidpf stream);
	static int ZCALLBACK errorFileProxy(voidpf opaque, voidpf stream);
};


VCMI_LIB_NAMESPACE_END
