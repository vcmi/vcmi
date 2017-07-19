/*
 * BinaryDeserializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BinaryDeserializer.h"

#include "../registerTypes/RegisterTypes.h"

extern template void registerTypes<BinaryDeserializer>(BinaryDeserializer & s);

CLoadFile::CLoadFile(const boost::filesystem::path & fname, int minimalVersion)
	: serializer(this)
{
	registerTypes(serializer);
	openNextFile(fname, minimalVersion);
}

CLoadFile::~CLoadFile()
{
}

int CLoadFile::read(void * data, unsigned size)
{
	sfile->read((char *)data, size);
	return size;
}

void CLoadFile::openNextFile(const boost::filesystem::path & fname, int minimalVersion)
{
	assert(!serializer.reverseEndianess);
	assert(minimalVersion <= SERIALIZATION_VERSION);

	try
	{
		fName = fname.string();
		sfile = make_unique<FileStream>(fname, std::ios::in | std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to read %s!", fName);

		//we can read
		char buffer[4];
		sfile->read(buffer, 4);
		if(std::memcmp(buffer, "VCMI", 4))
			THROW_FORMAT("Error: not a VCMI file(%s)!", fName);

		serializer & serializer.fileVersion;
		if(serializer.fileVersion < minimalVersion)
			THROW_FORMAT("Error: too old file format (%s)!", fName);

		if(serializer.fileVersion > SERIALIZATION_VERSION)
		{
			logGlobal->warnStream() << boost::format("Warning format version mismatch: found %d when current is %d! (file %s)\n") % serializer.fileVersion % SERIALIZATION_VERSION % fName;

			auto versionptr = (char *)&serializer.fileVersion;
			std::reverse(versionptr, versionptr + 4);
			logGlobal->warnStream() << "Version number reversed is " << serializer.fileVersion << ", checking...";

			if(serializer.fileVersion == SERIALIZATION_VERSION)
			{
				logGlobal->warnStream() << fname << " seems to have different endianness! Entering reversing mode.";
				serializer.reverseEndianess = true;
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

void CLoadFile::reportState(CLogger * out)
{
	out->debugStream() << "CLoadFile";
	if(!!sfile && *sfile)
	{
		out->debugStream() << "\tOpened " << fName << "\n\tPosition: " << sfile->tellg();
	}
}

void CLoadFile::clear()
{
	sfile = nullptr;
	fName.clear();
	serializer.fileVersion = 0;
}

void CLoadFile::checkMagicBytes(const std::string & text)
{
	std::string loaded = text;
	read((void *)loaded.data(), text.length());
	if(loaded != text)
		throw std::runtime_error("Magic bytes doesn't match!");
}
