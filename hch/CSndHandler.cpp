#include "../stdafx.h"
#include <fstream>
#include "CSndHandler.h"
#include <boost/iostreams/device/mapped_file.hpp>

/*
 * CSndHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CMediaHandler::~CMediaHandler()
{
	entries.clear();
	fimap.clear();
	mfile->close();
	delete mfile;
}

// Analyze the sound file. Half of this could go away if we were using
// a simple structure. However, some post treatment would be necessary: file
// size and offsets are little endian, and filename have a NUL in
// them. */
CMediaHandler::CMediaHandler(std::string fname)
{
	try //c-tor of mapped_file_source throws exception on failure
	{
		mfile = new boost::iostreams::mapped_file_source(fname);
	} HANDLE_EXCEPTION
	if (!mfile->is_open()) //just in case
	{
		tlog1 << "Cannot open " << fname << std::endl;
		throw std::string("Cannot open ")+fname;
	}
}

// Reads a 4 byte integer. Format on file is little endian.
unsigned int CMediaHandler::readNormalNr (const unsigned char *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void CMediaHandler::extract(int index, std::string dstfile) //saves selected file
{
	std::ofstream out(dstfile.c_str(),std::ios_base::binary);
	const char *data = mfile->data();
	
	out.write(&data[entries[index].offset], entries[index].size);
	out.close();
}

void CMediaHandler::extract(std::string srcfile, std::string dstfile, bool caseSens) //saves selected file
{
	if (caseSens)
	{
		for (size_t i=0;i<entries.size();++i)
		{
			if (entries[i].name==srcfile)
				extract(i,dstfile);
		}
	}
	else
	{
		std::transform(srcfile.begin(),srcfile.end(),srcfile.begin(),tolower);
		for (size_t i=0;i<entries.size();++i)
		{
			if (entries[i].name==srcfile)
			{
				std::string por = entries[i].name;
				std::transform(por.begin(),por.end(),por.begin(),tolower);
				if (por==srcfile)
					extract(i,dstfile);
			}
		}
	}
}

#if 0
// unused and not sure what it's supposed to do
MemberFile CMediaHandler::getFile(std::string name)
{
	MemberFile ret;
	std::transform(name.begin(),name.end(),name.begin(),tolower);
	for (size_t i=0;i<entries.size();++i)
	{
		if (entries[i].name==name)
		{
			std::string por = entries[i].name;
			std::transform(por.begin(),por.end(),por.begin(),tolower);
			if (por==name)
			{
				ret.length=entries[i].size;
				file.seekg(entries[i].offset,std::ios_base::beg);
				ret.ifs=&file;
				return ret;
			}
		}
	}
	return ret;
}
#endif

const char * CMediaHandler::extract (int index, int & size)
{
	size = entries[index].size;
	const char *data = mfile->data();

	return &data[entries[index].offset];
}

const char * CMediaHandler::extract (std::string srcName, int &size)
{
	int index;
	std::map<std::string, int>::iterator fit;
	if ((fit = fimap.find(srcName)) != fimap.end())
	{
		index = fit->second;
		return this->extract(index, size);
	}
	size = 0;
	return NULL;
}

CSndHandler::CSndHandler(std::string fname) : CMediaHandler::CMediaHandler(fname)
{
	const unsigned char *data = (const unsigned char *)mfile->data();

	unsigned int numFiles = readNormalNr(&data[0]);

	for (unsigned int i=0; i<numFiles; i++)
	{
		Entry entry;
		const unsigned char *p;

		// Read file name and extension
		p = &data[4+48*i];

		while(*p) {
			entry.name += *p;
			p++;
		}

		entry.name+='.';
		p++;

		while(*p)
		{
			entry.name += *p;
			p++;
		}

		// Read offset and size
		p = &data[4+48*i+40];
		entry.offset = readNormalNr(p);

		p += 4;
		entry.size = readNormalNr(p);

		entries.push_back(entry);
		fimap[entry.name] = i;
	}
}

CVidHandler::CVidHandler(std::string fname) : CMediaHandler::CMediaHandler(fname) 
{
	const unsigned char *data = (const unsigned char *)mfile->data();

	unsigned int numFiles = readNormalNr(&data[0]);

	for (unsigned int i=0; i<numFiles; i++)
	{
		Entry entry;
		const unsigned char *p;

		// Read file name and extension
		p = &data[4+44*i];

		while(*p) {
			entry.name += *p;
			p++;
		}

		entry.name+='.';
		p++;

		while(*p)
		{
			entry.name += *p;
			p++;
		}

		// Read offset and size
		p = &data[4+44*i+40];
		entry.offset = readNormalNr(p);

		// There is no size, so check where the next file is
		if (i == numFiles - 1) {
			entry.size = mfile->size() - entry.offset;
		} else {
			p = &data[4+44*(i+1)+40];
			int next_offset = readNormalNr(p);
			entry.size = next_offset - entry.offset;
		}	

		entries.push_back(entry);
		fimap[entry.name] = i;
	}
}
