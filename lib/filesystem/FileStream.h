/*
 * FileStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/stream.hpp>

class DLL_LINKAGE FileBuf
{
public:
	typedef char char_type;
	typedef struct category_ : boost::iostreams::seekable_device_tag, boost::iostreams::closable_tag
	{} category;

	FileBuf(const boost::filesystem::path & filename, std::ios_base::openmode mode);

	std::streamsize read(char * s, std::streamsize n);
	std::streamsize write(const char * s, std::streamsize n);
	std::streamoff seek(std::streamoff off, std::ios_base::seekdir way);

	void close();

private:
	void * filePtr;
};

struct zlib_filefunc64_def_s;
typedef zlib_filefunc64_def_s zlib_filefunc64_def;

#ifdef VCMI_DLL
extern template struct DLL_LINKAGE boost::iostreams::stream<FileBuf>;
#endif

class DLL_LINKAGE FileStream : public boost::iostreams::stream<FileBuf>
{
public:
	FileStream() = default;
	explicit FileStream(const boost::filesystem::path & p, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
		: boost::iostreams::stream<FileBuf>(p, mode) {}

	static bool CreateFile(const boost::filesystem::path & filename);

	static zlib_filefunc64_def * GetMinizipFilefunc();
};
