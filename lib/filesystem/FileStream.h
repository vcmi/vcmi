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

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE FileBuf
{
public:
	using char_type = char;
	using category = struct category_ :
		boost::iostreams::seekable_device_tag,
		boost::iostreams::closable_tag
		{};

	FileBuf(const boost::filesystem::path& filename, std::ios_base::openmode mode);

	std::streamsize read(char* s, std::streamsize n);
	std::streamsize write(const char* s, std::streamsize n);
	std::streamoff  seek(std::streamoff off, std::ios_base::seekdir way);

	void close();
private:
	void* filePtr;
};

VCMI_LIB_NAMESPACE_END

struct zlib_filefunc64_def_s;
using zlib_filefunc64_def = zlib_filefunc64_def_s;

#ifdef VCMI_DLL
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4910)
#endif
extern template struct DLL_LINKAGE boost::iostreams::stream<VCMI_LIB_WRAP_NAMESPACE(FileBuf)>;
#ifdef _MSC_VER
#pragma warning (pop)
#endif
#endif

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE FileStream : public boost::iostreams::stream<FileBuf>
{
public:
	FileStream() = default;
	explicit FileStream(const boost::filesystem::path& p, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
		: boost::iostreams::stream<FileBuf>(p, mode) {}

	static bool createFile(const boost::filesystem::path& filename);

	static zlib_filefunc64_def* GetMinizipFilefunc();
};

VCMI_LIB_NAMESPACE_END
