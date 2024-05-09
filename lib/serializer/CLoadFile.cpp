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

CLoadFile::CLoadFile(const boost::filesystem::path & fname, ESerializationVersion minimalVersion)
	: serializer(this)
{
	openNextFile(fname, minimalVersion);
}

//must be instantiated in .cpp file for access to complete types of all member fields
CLoadFile::~CLoadFile() = default;

int CLoadFile::read(std::byte * data, unsigned size)
{
	sfile->read(reinterpret_cast<char *>(data), size);
	return size;
}

void CLoadFile::openNextFile(const boost::filesystem::path & fname, ESerializationVersion minimalVersion)
{
	assert(!serializer.reverseEndianness);
	assert(minimalVersion <= ESerializationVersion::CURRENT);

	try
	{
		fName = fname.string();
		sfile = std::make_unique<std::fstream>(fname.c_str(), std::ios::in | std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to read %s!", fName);

		//we can read
		char buffer[4];
		sfile->read(buffer, 4);
		if(std::memcmp(buffer, "VCMI", 4) != 0)
			THROW_FORMAT("Error: not a VCMI file(%s)!", fName);

		serializer & serializer.version;
		if(serializer.version < minimalVersion)
			THROW_FORMAT("Error: too old file format (%s)!", fName);

		if(serializer.version > ESerializationVersion::CURRENT)
		{
			logGlobal->warn("Warning format version mismatch: found %d when current is %d! (file %s)\n", vstd::to_underlying(serializer.version), vstd::to_underlying(ESerializationVersion::CURRENT), fName);

			auto * versionptr = reinterpret_cast<char *>(&serializer.version);
			std::reverse(versionptr, versionptr + 4);
			logGlobal->warn("Version number reversed is %x, checking...", vstd::to_underlying(serializer.version));

			if(serializer.version == ESerializationVersion::CURRENT)
			{
				logGlobal->warn("%s seems to have different endianness! Entering reversing mode.", fname.string());
				serializer.reverseEndianness = true;
			}
			else
				THROW_FORMAT("Error: too new file format (%s)!", fName);
		}
	}
	catch(...)
	{
		clear(); //if anything went wrong, we delete file and rethrow
		throw;
	}
}

void CLoadFile::reportState(vstd::CLoggerBase * out)
{
	out->debug("CLoadFile");
	if(!!sfile && *sfile)
		out->debug("\tOpened %s Position: %d", fName, sfile->tellg());
}

void CLoadFile::clear()
{
	sfile = nullptr;
	fName.clear();
	serializer.version = ESerializationVersion::NONE;
}

void CLoadFile::checkMagicBytes(const std::string &text)
{
	std::string loaded = text;
	read(reinterpret_cast<std::byte*>(loaded.data()), text.length());
	if(loaded != text)
		throw std::runtime_error("Magic bytes doesn't match!");
}

VCMI_LIB_NAMESPACE_END
