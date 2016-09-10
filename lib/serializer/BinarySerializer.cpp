#include "StdInc.h"
#include "BinarySerializer.h"

#include "../registerTypes/RegisterTypes.h"

/*
 * BinarySerializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

extern template void registerTypes<BinarySerializer>(BinarySerializer & s);

CSaveFile::CSaveFile(const boost::filesystem::path &fname): serializer(this)
{
	registerTypes(serializer);
	openNextFile(fname);
}

CSaveFile::~CSaveFile()
{
}

int CSaveFile::write( const void * data, unsigned size )
{
	sfile->write((char *)data,size);
	return size;
}

void CSaveFile::openNextFile(const boost::filesystem::path &fname)
{
	fName = fname;
	try
	{
		sfile = make_unique<FileStream>(fname, std::ios::out | std::ios::binary);
		sfile->exceptions(std::ifstream::failbit | std::ifstream::badbit); //we throw a lot anyway

		if(!(*sfile))
			THROW_FORMAT("Error: cannot open to write %s!", fname);

		sfile->write("VCMI",4); //write magic identifier
		serializer & version; //write format version
	}
	catch(...)
	{
		logGlobal->errorStream() << "Failed to save to " << fname;
		clear();
		throw;
	}
}

void CSaveFile::reportState(CLogger * out)
{
	out->debugStream() << "CSaveFile";
	if(sfile.get() && *sfile)
	{
		out->debugStream() << "\tOpened " << fName << "\n\tPosition: " << sfile->tellp();
	}
}

void CSaveFile::clear()
{
	fName.clear();
	sfile = nullptr;
}

void CSaveFile::putMagicBytes( const std::string &text )
{
	write(text.c_str(), text.length());
}
