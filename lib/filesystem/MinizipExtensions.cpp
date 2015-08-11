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

static voidpf ZCALLBACK openFileProxy(voidpf opaque, const void * filename, int mode)
{
	assert(opaque != nullptr);
	
	std::string filename_s;
	
	if(filename != nullptr)
		filename_s = (const char *)filename;
	
	return ((CIOApi *)opaque)->openFile(filename_s, mode);
}

static uLong ZCALLBACK readFileProxy(voidpf opaque, voidpf stream, void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
	
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);
	
	return actualStream->read((ui8 *)buf, size);
}

static uLong ZCALLBACK writeFileProxy(voidpf opaque, voidpf stream, const void * buf, uLong size)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);

	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);
    return (uLong)actualStream->write((const ui8 *)buf, size);
}

static ZPOS64_T ZCALLBACK tellFileProxy(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
	
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);	
    return actualStream->tell();
}

static long ZCALLBACK seekFileProxy(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
		
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);	

    long ret = 0;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        actualStream->skip(offset);//TODO: should we check actual skipped? 
        break;
    case ZLIB_FILEFUNC_SEEK_END :
    	ret = -1;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
    	ret = actualStream->seek(offset);
        break;
    default: ret = -1;
    }
    return ret;
}

static int ZCALLBACK closeFileProxy(voidpf opaque, voidpf stream)
{
	assert(opaque != nullptr);
	assert(stream != nullptr);
		
	CInputOutputStream * actualStream = static_cast<CInputOutputStream *>(stream);
	
	delete actualStream;
	
    return 0;
}

static int ZCALLBACK errorFileProxy(voidpf opaque, voidpf stream)
{
    return 0;
}

void CIOApi::fillApiStructure(zlib_filefunc64_def & api)
{
	api.opaque = this;
	api.zopen64_file = &openFileProxy;
	api.zread_file = &readFileProxy;
	api.zwrite_file = &writeFileProxy;
	api.ztell64_file = &tellFileProxy;
	api.zseek64_file = &seekFileProxy;
	api.zclose_file = &closeFileProxy;
	api.zerror_file = &errorFileProxy;	
}

