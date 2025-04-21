/*
 * CLoadFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CLoadFile.h"

VCMI_LIB_NAMESPACE_BEGIN

template <typename From, typename To>
struct static_caster
{
	To operator()(From p) {return static_cast<To>(p);}
};


CLoadFile::CLoadFile(const boost::filesystem::path & fname, IGameCallback * cb)
	: serializer(this)
	, sfile(fname.c_str(), std::ios::in | std::ios::binary)
{
	serializer.cb = cb;
	serializer.loadingGamestate = true;
	assert(!serializer.reverseEndianness);

	sfile.exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

	if(!sfile)
		throw std::runtime_error("Error: cannot open file '" + fname.string() + "' for reading!");

	static const std::string MAGIC = "VCMI";
	std::string readMagic = MAGIC;
	sfile.read(readMagic.data(), 4);
	if(readMagic != MAGIC)
		throw std::runtime_error("Error: '" + fname.string() + "' is not a VCMI file!");

	sfile.read(reinterpret_cast<char*>(&serializer.version), sizeof(serializer.version));

	if(serializer.version < ESerializationVersion::MINIMAL)
		throw std::runtime_error("Error: too old file format detected in '" + fname.string() + "'!");

	if(serializer.version > ESerializationVersion::CURRENT)
	{
		logGlobal->warn("Warning format version mismatch: found %d when current is %d! (file %s)\n", vstd::to_underlying(serializer.version), vstd::to_underlying(ESerializationVersion::CURRENT), fname.string());

		auto * versionptr = reinterpret_cast<char *>(&serializer.version);
		std::reverse(versionptr, versionptr + 4);
		logGlobal->warn("Version number reversed is %x, checking...", vstd::to_underlying(serializer.version));

		if(serializer.version == ESerializationVersion::CURRENT)
		{
			logGlobal->warn("%s seems to have different endianness! Entering reversing mode.", fname.string());
			serializer.reverseEndianness = true;
		}
		else
			throw std::runtime_error("Error: too new file format detected in '" + fname.string() + "'!");
	}

	std::string loaded = SAVEGAME_MAGIC;
	sfile.read(loaded.data(), SAVEGAME_MAGIC.length());
	if(loaded != SAVEGAME_MAGIC)
		throw std::runtime_error("Magic bytes doesn't match!");
}

int CLoadFile::read(std::byte * data, unsigned size)
{
	sfile.read(reinterpret_cast<char *>(data), size);
	return size;
}

VCMI_LIB_NAMESPACE_END
