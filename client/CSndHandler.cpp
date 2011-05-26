#include "../stdafx.h"
#include <fstream>
#include "CSndHandler.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <SDL_endian.h>

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

CMediaHandler::CMediaHandler(std::string fname)
{
	try //c-tor of mapped_file_source throws exception on failure
	{
		mfile = new boost::iostreams::mapped_file_source(fname);
	} HANDLE_EXCEPTIONC(tlog1 << "Cannot open " << fname << std::endl)
	if (!mfile->is_open()) //just in case
	{
		tlog1 << "Cannot open " << fname << std::endl;
		throw std::string("Cannot open ")+fname;
	}
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
	srcfile.erase(srcfile.find_last_of('.'));
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
	srcName.erase(srcName.find_last_of('.'));

	std::map<std::string, int>::iterator fit;
	if ((fit = fimap.find(srcName)) != fimap.end())
	{
		index = fit->second;
		return this->extract(index, size);
	}
	size = 0;
	return NULL;
}

CSndHandler::CSndHandler(std::string fname) : CMediaHandler(fname)
{
	const unsigned char *data = (const unsigned char *)mfile->data();
	unsigned int numFiles = SDL_SwapLE32(*(Uint32 *)&data[0]);
	struct soundEntry *se = (struct soundEntry *)&data[4];

	for (unsigned int i=0; i<numFiles; i++, se++)
	{
		Entry entry;
		//		char *p;

		// Reassemble the filename, drop extension
		entry.name = se->filename;
		//		entry.name += '.';
		//		p = se->filename;
		//		while(*p) p++;
		//		p++;
		//		entry.name += p;

		entry.offset = SDL_SwapLE32(se->offset);
		entry.size = SDL_SwapLE32(se->size);

		entries.push_back(entry);
		fimap[entry.name] = i;
	}
}

CVidHandler::CVidHandler(std::string fname) : CMediaHandler(fname) 
{
	if(mfile->size() < 48)
	{
		tlog1 << fname << " doesn't contain needed data!\n";
		return;
	}
	const unsigned char *data = (const unsigned char *)mfile->data();
	unsigned int numFiles = SDL_SwapLE32(*(Uint32 *)&data[0]);
	struct videoEntry *ve = (struct videoEntry *)&data[4];

	for (unsigned int i=0; i<numFiles; i++, ve++)
	{
		Entry entry;

		entry.name = ve->filename;
		entry.offset = SDL_SwapLE32(ve->offset);
		entry.name.erase(entry.name.find_last_of('.'));

		// There is no size, so check where the next file is
		if (i == numFiles - 1) {
			entry.size = mfile->size() - entry.offset;
		} else {
			struct videoEntry *ve_next = ve+1;

			entry.size = SDL_SwapLE32(ve_next->offset) - entry.offset;
		}

		entries.push_back(entry);
		fimap[entry.name] = i;
	}
}
