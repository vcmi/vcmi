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


///CIOApi
voidpf ZCALLBACK CIOApi::openFileProxy(voidpf opaque, const void * filename, int mode)
{
	assert(opaque != nullptr);
	
	std::string filename_s;
	
	if(filename != nullptr)
		filename_s = (const char *)filename;
	
	return ((CIOApi *)opaque)->openFile(filename_s, mode);
}

uLong ZCALLBACK CIOApi::readFileProxy(voidpf opaque, voidpf stream, void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
	
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);
	
	return actualStream->read((ui8 *)buf, size);
}

uLong ZCALLBACK CIOApi::writeFileProxy(voidpf opaque, voidpf stream, const void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);
    return (uLong)actualStream->write((const ui8 *)buf, size);
}

ZPOS64_T ZCALLBACK CIOApi::tellFileProxy(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
	
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);	
    return actualStream->tell();
}

long ZCALLBACK CIOApi::seekFileProxy(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
		
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);	

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
		logGlobal->error("CIOApi::seekFileProxy: seek failed");			
    return ret;
}

int ZCALLBACK CIOApi::closeFileProxy(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
		
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);
	
	((CIOApi *)opaque)->closeFile(actualStream);
	
    return 0;
}

int ZCALLBACK CIOApi::errorFileProxy(voidpf opaque, voidpf stream)
{
    return 0;
}

zlib_filefunc64_def CIOApi::getApiStructure() const
{
	zlib_filefunc64_def api;
	api.opaque = (void *) this;
	api.zopen64_file = &openFileProxy;
	api.zread_file = &readFileProxy;
	api.zwrite_file = &writeFileProxy;
	api.ztell64_file = &tellFileProxy;
	api.zseek64_file = &seekFileProxy;
	api.zclose_file = &closeFileProxy;
	api.zerror_file = &errorFileProxy;	
	
	return api;
}

void CIOApi::closeFile(CInputOutputStream * stream) const
{
	delete stream;
}

///CDefaultIOApi
CDefaultIOApi::CDefaultIOApi()
{
	
}

CDefaultIOApi::~CDefaultIOApi()
{
	
}

zlib_filefunc64_def CDefaultIOApi::getApiStructure() const
{
	zlib_filefunc64_def api;
	
	fill_fopen64_filefunc(&api);
	
	return api;	
}

CInputOutputStream * CDefaultIOApi::openFile(const std::string& filename, int mode) const
{
	throw new std::runtime_error("CDefaultIOApi::openFile call not expected.");	
}

///CProxyIOApi
CProxyIOApi::CProxyIOApi(CInputOutputStream * buffer):
	data(buffer)
{
	
}

CProxyIOApi::~CProxyIOApi()
{
	
}

CInputOutputStream * CProxyIOApi::openFile(const std::string& filename, int mode) const
{
	logGlobal->traceStream() << "CProxyIOApi: stream opened for " <<filename<<" with mode "<<mode; 
	
	data->seek(0);
	return data;
}

void CProxyIOApi::closeFile(CInputOutputStream * stream) const
{
	logGlobal->traceStream() << "CProxyIOApi: stream closed";
	stream->seek(0);//stream is local singleton and even not owned, just seek
}
