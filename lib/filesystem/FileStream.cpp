#include "StdInc.h"
#include "FileStream.h"

#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#include <cwchar>
	#define DO_OPEN(name, mode) _wfopen(name, L##mode)
#else
	#define DO_OPEN(name, mode) fopen(name, mode)
#endif

#include <cstdio>

#define GETFILE static_cast<std::FILE*>(filePtr)

/*static*/
bool FileStream::CreateFile(const boost::filesystem::path& filename)
{
	FILE* f = DO_OPEN(filename.c_str(), "wb");
	bool result = (f != nullptr);
	fclose(f);
	return result;
}

FileBuf::FileBuf(const boost::filesystem::path& filename, std::ios_base::openmode mode)
{
	std::string openmode = [mode]() -> std::string
	{
		using namespace std;
		switch (mode & (~ios_base::ate & ~ios_base::binary))
		{
		case (ios_base::in):
			return "r";
		case (ios_base::out):
		case (ios_base::out | ios_base::trunc):
			return "w";
		case (ios_base::app):
		case (ios_base::out | ios_base::app):
			return "a";
		case (ios_base::out | ios_base::in):
			return "r+";
		case (ios_base::out | ios_base::in | ios_base::trunc):
			return "w+";
		case (ios_base::out | ios_base::in | ios_base::app):
		case (ios_base::in | ios_base::app):
			return "a+";
		default:
			throw std::ios_base::failure("invalid open mode");
		}
	}();

	if (mode & std::ios_base::binary)
		openmode += 'b';

	#if defined(_WIN32)
	filePtr = _wfopen(filename.c_str(), std::wstring(openmode.begin(), openmode.end()).c_str());
	#else
	filePtr = std::open(filename.c_str(), openmode);
	#endif

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
	auto src = [way]() -> int
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
	if(std::fseek(GETFILE, off, src))
		throw std::ios_base::failure("bad seek offset");

	return static_cast<std::streamsize>(std::ftell(GETFILE));
}
