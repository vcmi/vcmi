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

CSndHandler::~CSndHandler()
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
CSndHandler::CSndHandler(std::string fname)
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

// Reads a 4 byte integer. Format on file is little endian.
unsigned int CSndHandler::readNormalNr (const unsigned char *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

void CSndHandler::extract(int index, std::string dstfile) //saves selected file
{
	std::ofstream out(dstfile.c_str(),std::ios_base::binary);
	const char *data = mfile->data();
	
	out.write(&data[entries[index].offset], entries[index].size);
	out.close();
}

void CSndHandler::extract(std::string srcfile, std::string dstfile, bool caseSens) //saves selected file
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
MemberFile CSndHandler::getFile(std::string name)
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

const char * CSndHandler::extract (int index, int & size)
{
	size = entries[index].size;
	const char *data = mfile->data();

	return &data[entries[index].offset];
}

const char * CSndHandler::extract (std::string srcName, int &size)
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


CVidHandler::~CVidHandler()
{
	entries.clear();
	file.close();
}
CVidHandler::CVidHandler(std::string fname):CHUNK(65535)
{
	file.open(fname.c_str(),std::ios::binary);
	if (!file.is_open())
#ifndef __GNUC__
		throw new std::exception((std::string("Cannot open ")+fname).c_str());
#else
		throw new std::exception();
#endif
	int nr = readNormalNr(0,4);
	char tempc;
	for (int i=0;i<nr;i++)
	{
		Entry entry;
		while(true)
		{
			file.read(&tempc,1);
			if (tempc)
				entry.name+=tempc;
			else break;
		}
		entry.something=readNormalNr(-1,4);
		file.seekg(44-entry.name.length()-9,std::ios_base::cur);
		entry.offset = readNormalNr(-1,4);
		if (i>0)
			entries[i-1].size=entry.offset-entries[i-1].offset;
		if (i==nr-1)
		{
			file.seekg(0,std::ios::end);
			entry.size = ((int)file.tellg())-entry.offset;
			file.seekg(0,std::ios::beg);
		}
		entries.push_back(entry);
	}
}
int CVidHandler::readNormalNr (int pos, int bytCon)
{
	if (pos>=0)
		file.seekg(pos,std::ios_base::beg);
	int ret=0;
	int amp=1;
	unsigned char zcz=0;
	for (int i=0; i<bytCon; i++)
	{
		file.read((char*)(&zcz),1);
		ret+=zcz*amp;
		amp*=256;
	}
	return ret;
}
void CVidHandler::extract(int index, std::string dstfile) //saves selected file
{
	std::ofstream out(dstfile.c_str(),std::ios_base::binary);
	file.seekg(entries[index].offset,std::ios_base::beg);
	int toRead=entries[index].size;
	char * buffer = new char[std::min(CHUNK,entries[index].size)];
	while (toRead>CHUNK)
	{
		file.read(buffer,CHUNK);
		out.write(buffer,CHUNK);
		toRead-=CHUNK;
	}
	file.read(buffer,toRead);
	out.write(buffer,toRead);
	out.close();
}
void CVidHandler::extract(std::string srcfile, std::string dstfile, bool caseSens) //saves selected file
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
MemberFile CVidHandler::getFile(std::string name)
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
	throw ret;
	//return ret;
}
