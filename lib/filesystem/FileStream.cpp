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
#include <minizip/ioapi.h>
#else
#include "../minizip/unzip.h"
#include "../minizip/ioapi.h"
#endif

#include <cstdio>

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
