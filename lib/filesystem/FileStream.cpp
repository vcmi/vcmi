/*
 * FileStream.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "FileStream.h"

#ifdef USE_SYSTEM_MINIZIP
#include <minizip/unzip.h>
#else
#include "../minizip/unzip.h"
#endif

#include <cstdio>

///copied from ioapi.c due to linker issues on MSVS

#include "../minizip/ioapi.h"

#if defined(__APPLE__) || defined(IOAPI_NO_64)
// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif

voidpf call_zopen64 (const zlib_filefunc64_32_def* pfilefunc,const void*filename,int mode)
{
	if (pfilefunc->zfile_func64.zopen64_file != NULL)
		return (*(pfilefunc->zfile_func64.zopen64_file)) (pfilefunc->zfile_func64.opaque,filename,mode);
	else
	{
		return (*(pfilefunc->zopen32_file))(pfilefunc->zfile_func64.opaque,(const char*)filename,mode);
	}
}

long call_zseek64(const zlib_filefunc64_32_def* pfilefunc, voidpf filestream, ZPOS64_T offset, int origin)
{
	if (pfilefunc->zfile_func64.zseek64_file != NULL)
		return (*(pfilefunc->zfile_func64.zseek64_file)) (pfilefunc->zfile_func64.opaque, filestream, offset, origin);
	else
	{
		uLong offsetTruncated = (uLong)offset;
		if (offsetTruncated != offset)
			return -1;
		else
			return (*(pfilefunc->zseek32_file))(pfilefunc->zfile_func64.opaque, filestream, offsetTruncated, origin);
	}
}

ZPOS64_T call_ztell64(const zlib_filefunc64_32_def* pfilefunc, voidpf filestream)
{
	if (pfilefunc->zfile_func64.zseek64_file != NULL)
		return (*(pfilefunc->zfile_func64.ztell64_file)) (pfilefunc->zfile_func64.opaque, filestream);
	else
	{
		uLong tell_uLong = (*(pfilefunc->ztell32_file))(pfilefunc->zfile_func64.opaque, filestream);
		if ((tell_uLong) == MAXU32)
			return (ZPOS64_T)-1;
		else
			return tell_uLong;
	}
}

static uLong   ZCALLBACK fread_file_func OF((voidpf opaque, voidpf stream, void* buf, uLong size));
static uLong   ZCALLBACK fwrite_file_func OF((voidpf opaque, voidpf stream, const void* buf,uLong size));
static ZPOS64_T ZCALLBACK ftell64_file_func OF((voidpf opaque, voidpf stream));
static long    ZCALLBACK fseek64_file_func OF((voidpf opaque, voidpf stream, ZPOS64_T offset, int origin));
static int     ZCALLBACK fclose_file_func OF((voidpf opaque, voidpf stream));
static int     ZCALLBACK ferror_file_func OF((voidpf opaque, voidpf stream));

static voidpf ZCALLBACK fopen64_file_func(voidpf opaque, const void* filename, int mode)
{
	FILE* file = NULL;
	const char* mode_fopen = NULL;
	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
		mode_fopen = "rb";
	else
		if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
			mode_fopen = "r+b";
		else
			if (mode & ZLIB_FILEFUNC_MODE_CREATE)
				mode_fopen = "wb";

	if ((filename != NULL) && (mode_fopen != NULL))
		file = FOPEN_FUNC((const char*)filename, mode_fopen);
	return file;
}


static uLong ZCALLBACK fread_file_func(voidpf opaque, voidpf stream, void* buf, uLong size)
{
	uLong ret;
	ret = (uLong)fread(buf, 1, (size_t)size, (FILE *)stream);
	return ret;
}

static uLong ZCALLBACK fwrite_file_func(voidpf opaque, voidpf stream, const void* buf, uLong size)
{
	uLong ret;
	ret = (uLong)fwrite(buf, 1, (size_t)size, (FILE *)stream);
	return ret;
}

static ZPOS64_T ZCALLBACK ftell64_file_func(voidpf opaque, voidpf stream)
{
	ZPOS64_T ret;
	ret = FTELLO_FUNC((FILE *)stream);
	return ret;
}

static long ZCALLBACK fseek64_file_func(voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin)
{
	int fseek_origin = 0;
	long ret;
	switch (origin)
	{
	case ZLIB_FILEFUNC_SEEK_CUR:
		fseek_origin = SEEK_CUR;
		break;
	case ZLIB_FILEFUNC_SEEK_END:
		fseek_origin = SEEK_END;
		break;
	case ZLIB_FILEFUNC_SEEK_SET:
		fseek_origin = SEEK_SET;
		break;
	default: return -1;
	}
	ret = 0;

	if (FSEEKO_FUNC((FILE *)stream, offset, fseek_origin) != 0)
		ret = -1;

	return ret;
}


static int ZCALLBACK fclose_file_func(voidpf opaque, voidpf stream)
{
	int ret;
	ret = fclose((FILE *)stream);
	return ret;
}

static int ZCALLBACK ferror_file_func(voidpf opaque, voidpf stream)
{
	int ret;
	ret = ferror((FILE *)stream);
	return ret;
}

///end of ioapi.c

//extern MINIZIP_EXPORT void fill_fopen64_filefunc(zlib_filefunc64_def*  pzlib_filefunc_def);

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

inline FILE* do_open(const CharType* name, const CharType* mode)
{
	#ifdef VCMI_WINDOWS
		return _wfopen(name, mode);
	#else
		return std::fopen(name, mode);
	#endif
}

#define GETFILE static_cast<std::FILE*>(filePtr)

voidpf ZCALLBACK MinizipOpenFunc(voidpf opaque, const void* filename, int mode)
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


void fill_fopen64_filefunc(zlib_filefunc64_def*  pzlib_filefunc_def)
{
	pzlib_filefunc_def->zopen64_file = fopen64_file_func;
	pzlib_filefunc_def->zread_file = fread_file_func;
	pzlib_filefunc_def->zwrite_file = fwrite_file_func;
	pzlib_filefunc_def->ztell64_file = ftell64_file_func;
	pzlib_filefunc_def->zseek64_file = fseek64_file_func;
	pzlib_filefunc_def->zclose_file = fclose_file_func;
	pzlib_filefunc_def->zerror_file = ferror_file_func;
	pzlib_filefunc_def->opaque = NULL;
}


template struct boost::iostreams::stream<VCMI_LIB_WRAP_NAMESPACE(FileBuf)>;

VCMI_LIB_NAMESPACE_BEGIN

zlib_filefunc64_def* FileStream::GetMinizipFilefunc()
{
	static zlib_filefunc64_def MinizipFilefunc;
	static bool initialized = false;
	if (!initialized)
	{
		fill_fopen64_filefunc(&MinizipFilefunc);
		MinizipFilefunc.zopen64_file = &MinizipOpenFunc;
		initialized = true;
	}
	return &MinizipFilefunc;
}

/*static*/
bool FileStream::createFile(const boost::filesystem::path& filename)
{
	FILE* f = do_open(filename.c_str(), CHAR_LITERAL("wb"));
	bool result = (f != nullptr);
	if(result)
		fclose(f);
	return result;
}

FileBuf::FileBuf(const boost::filesystem::path& filename, std::ios_base::openmode mode)
{
	auto openmode = [mode]() -> std::basic_string<CharType>
	{
		using namespace std;
		switch (mode & (~ios_base::ate & ~ios_base::binary))
		{
		case (ios_base::in):
			return CHAR_LITERAL("r");
		case (ios_base::out):
		case (ios_base::out | ios_base::trunc):
			return CHAR_LITERAL("w");
		case (ios_base::app):
		case (ios_base::out | ios_base::app):
			return CHAR_LITERAL("a");
		case (ios_base::out | ios_base::in):
			return CHAR_LITERAL("r+");
		case (ios_base::out | ios_base::in | ios_base::trunc):
			return CHAR_LITERAL("w+");
		case (ios_base::out | ios_base::in | ios_base::app):
		case (ios_base::in | ios_base::app):
			return CHAR_LITERAL("a+");
		default:
			throw std::ios_base::failure("invalid open mode");
		}
	}();

	if (mode & std::ios_base::binary)
		openmode += CHAR_LITERAL('b');

	filePtr = do_open(filename.c_str(), openmode.c_str());

	if (filePtr == nullptr)
		throw std::ios_base::failure("could not open file");

	if (mode & std::ios_base::ate) {
		if (std::fseek(GETFILE, 0, SEEK_END)) {
			fclose(GETFILE);
			throw std::ios_base::failure("could not open file");
		}
	}
}

void FileBuf::close()
{
    std::fclose(GETFILE);
}

std::streamsize FileBuf::read(char* s, std::streamsize n)
{
	return static_cast<std::streamsize>(std::fread(s, 1, n, GETFILE));
}

std::streamsize FileBuf::write(const char* s, std::streamsize n)
{
	return static_cast<std::streamsize>(std::fwrite(s, 1, n, GETFILE));
}

std::streamoff FileBuf::seek(std::streamoff off, std::ios_base::seekdir way)
{
	const auto src = [way]() -> int
	{
		switch(way)
		{
		case std::ios_base::beg:
			return SEEK_SET;
		case std::ios_base::cur:
			return SEEK_CUR;
		case std::ios_base::end:
			return SEEK_END;
		default:
			throw std::ios_base::failure("bad seek direction");
		}
	}();
	if(std::fseek(GETFILE, (long)off, src))
		throw std::ios_base::failure("bad seek offset");

	return static_cast<std::streamsize>(std::ftell(GETFILE));
}

VCMI_LIB_NAMESPACE_END
