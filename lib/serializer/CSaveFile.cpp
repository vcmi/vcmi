/*
 * CSaveFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CSaveFile.h"

VCMI_LIB_NAMESPACE_BEGIN

CSaveFile::CSaveFile(const boost::filesystem::path &fname)
	: serializer(this)
	, sfile(fname.c_str(), std::ios::out | std::ios::binary)
{
	sfile.exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

	if(!sfile)
		throw std::runtime_error("Error: cannot open file '" + fname.string() + "' for writing!");

	sfile.write("VCMI", 4); //write magic identifier
	serializer & ESerializationVersion::CURRENT; //write format version
	sfile.write(SAVEGAME_MAGIC.c_str(), SAVEGAME_MAGIC.length());
}

int CSaveFile::write(const std::byte * data, unsigned size)
{
	sfile.write(reinterpret_cast<const char *>(data), size);
	return size;
}

VCMI_LIB_NAMESPACE_END
