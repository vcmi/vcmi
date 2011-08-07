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

/* Media file are kept in container files. We map these files in
 * memory, parse them and create an index of them to easily retrieve
 * the data+size of the objects. */

CMediaHandler::~CMediaHandler()
{
	std::vector<boost::iostreams::mapped_file_source *>::iterator it;

	entries.clear();
	fimap.clear();
	
	for (it=mfiles.begin() ; it < mfiles.end(); it++ ) {
		(*it)->close();
		delete *it;
	}
}

boost::iostreams::mapped_file_source *CMediaHandler::add_file(std::string fname)
{
	boost::iostreams::mapped_file_source *mfile;

	try //c-tor of mapped_file_source throws exception on failure
	{
		mfile = new boost::iostreams::mapped_file_source(fname);
	} HANDLE_EXCEPTIONC(tlog1 << "Cannot open " << fname << std::endl)
	if (!mfile->is_open()) //just in case
	{
		tlog1 << "Cannot open " << fname << std::endl;
		throw std::string("Cannot open ")+fname;
		return NULL;
	} else {
		mfiles.push_back(mfile);
		return mfile;
	}
}

void CMediaHandler::extract(int index, std::string dstfile) //saves selected file
{
	std::ofstream out(dstfile.c_str(),std::ios_base::binary);
	Entry &entry = entries[index];

	out.write(entry.data, entry.size);
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
	Entry &entry = entries[index];

	size = entry.size;
	return entry.data;
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

void CSndHandler::add_file(std::string fname)
{
	boost::iostreams::mapped_file_source *mfile = CMediaHandler::add_file(fname);
	if (!mfile)
		/* File doesn't exist. Silently skip it.*/
		return;

	const char *data = mfile->data();
	unsigned int numFiles = SDL_SwapLE32(*(Uint32 *)&data[0]);
	struct soundEntry *se = (struct soundEntry *)&data[4];

	for (unsigned int i=0; i<numFiles; i++, se++)
	{
		Entry entry;

		entry.name = se->filename;
		entry.offset = SDL_SwapLE32(se->offset);
		entry.size = SDL_SwapLE32(se->size);
		entry.data = mfile->data() + entry.offset;

		entries.push_back(entry);
		fimap[entry.name] = i;
	}
}

void CVidHandler::add_file(std::string fname)
{
	boost::iostreams::mapped_file_source *mfile = CMediaHandler::add_file(fname);
	if (!mfile)
		/* File doesn't exist. Silently skip it.*/
		return;

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
		entry.data = mfile->data() + entry.offset;

		entries.push_back(entry);
		fimap[entry.name] = i;
	}
}
