#pragma once

#include <iosfwd>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>

class DLL_LINKAGE FileBuf {
public:
	typedef char                 					char_type;
    typedef struct category_ :
    	boost::iostreams::seekable_device_tag,
    	boost::iostreams::closable_tag
    	{} category;

	FileBuf(const boost::filesystem::path& filename, std::ios_base::openmode mode = (std::ios_base::in | std::ios_base::out));

    std::streamsize read(char* s, std::streamsize n);
    std::streamsize write(const char* s, std::streamsize n);
    std::streamoff  seek(std::streamoff off, std::ios_base::seekdir way);

    void close();
private:
	void* filePtr;
};

struct zlib_filefunc64_def_s;
typedef zlib_filefunc64_def_s zlib_filefunc64_def;

#ifdef VCMI_DLL
extern template class DLL_LINKAGE boost::iostreams::stream<FileBuf>;
#else
template class DLL_LINKAGE boost::iostreams::stream<FileBuf>;
#endif

class DLL_LINKAGE FileStream : public boost::iostreams::stream<FileBuf>
{
public:
	using boost::iostreams::stream<FileBuf>::stream;

	static bool CreateFile(const boost::filesystem::path& filename);

	static zlib_filefunc64_def* GetMinizipFilefunc();
};
