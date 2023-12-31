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
{
	openNextFile(fname);
}

//must be instantiated in .cpp file for access to complete types of all member fields
CSaveFile::~CSaveFile() = default;

int CSaveFile::write(const void * data, unsigned size)
{
	sfile->write((char *)data,size);
	return size;
}

void CSaveFile::openNextFile(const boost::filesystem::path &fname)
{
	fName = fname;
	try
	{
		sfile = std::make_unique<std::fstream>(fname.c_str(), std::ios::out | std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to write %s!", fname);

		sfile->write("VCMI",4); //write magic identifier
		serializer & SERIALIZATION_VERSION; //write format version
	}
	catch(...)
	{
		logGlobal->error("Failed to save to %s", fname.string());
		clear();
		throw;
	}
}

void CSaveFile::reportState(vstd::CLoggerBase * out)
{
	out->debug("CSaveFile");
	if(sfile.get() && *sfile)
	{
		out->debug("\tOpened %s \tPosition: %d", fName, sfile->tellp());
	}
}

void CSaveFile::clear()
{
	fName.clear();
	sfile = nullptr;
}

void CSaveFile::putMagicBytes(const std::string &text)
{
	write(text.c_str(), static_cast<unsigned int>(text.length()));
}

VCMI_LIB_NAMESPACE_END
