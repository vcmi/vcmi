#define VCMI_DLL
#include "../stdafx.h"
#include "zlib.h"
#include "CLodHandler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include "boost/filesystem.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <SDL_endian.h>
#ifdef max
#undef max
#endif

/*
 * CLodHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

int readNormalNr (const unsigned char * bufor, int pos, int bytCon, bool cyclic)
{
	int ret=0;
	int amp=1;
	for (int ir=0; ir<bytCon; ir++)
	{
		ret+=bufor[pos+ir]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}

char readChar(const unsigned char * bufor, int &i)
{
	return bufor[i++];
}

std::string readString(const unsigned char * bufor, int &i)
{					
	int len = readNormalNr(bufor,i); i+=4;
	assert(len >= 0 && len <= 500000); //not too long
	std::string ret; ret.reserve(len);
	for(int gg=0; gg<len; ++gg)
	{
		ret += bufor[i++];
	}
	return ret;
}

void CLodHandler::convertName(std::string &filename, std::string *extension)
{
	std::transform(filename.begin(), filename.end(), filename.begin(), toupper);

	size_t dotPos = filename.find_last_of("/.");

	if ( dotPos != std::string::npos && filename[dotPos] == '.')
	{
		if (extension)
			*extension = filename.substr(dotPos);
		filename.erase(dotPos);
	}
}

unsigned char * CLodHandler::giveFile(std::string fname, LodFileType type, int * length)
{
	convertName(fname);
	boost::unordered_set<Entry>::const_iterator en_it = entries.find(Entry(fname, type));
	
	if(en_it == entries.end()) //nothing's been found
	{
		tlog1 << "Cannot find file: " << fname << std::endl;
		return NULL;
	}
	Entry ourEntry = *en_it;

	if(length) *length = ourEntry.realSize;
	mutex->lock();

	unsigned char * outp;
	if (ourEntry.offset<0) //file is in the sprites/ folder; no compression
	{
		int result;
		outp = new unsigned char[ourEntry.realSize];
		FILE * f = fopen((myDir + "/" + ourEntry.realName).c_str(), "rb");
		if (f)
		{
			result = fread(outp,1,ourEntry.realSize,f);
			fclose(f);
		}
		else
			result = -1;
		mutex->unlock();
		if(result<0)
		{
			tlog1<<"Error in file reading: " << myDir << "/" << ourEntry.name << std::endl;
			delete[] outp;
			return NULL;
		}
		else
			return outp;
	}
	else if (ourEntry.size==0) //file is not compressed
	{
		outp = new unsigned char[ourEntry.realSize];

		LOD.seekg(ourEntry.offset, std::ios::beg);
		LOD.read((char*)outp, ourEntry.realSize);
		mutex->unlock();
		return outp;
	}
	else //we will decompress file
	{
		outp = new unsigned char[ourEntry.size];

		LOD.seekg(ourEntry.offset, std::ios::beg);
		LOD.read((char*)outp, ourEntry.size);
		unsigned char * decomp = NULL;
		infs2(outp, ourEntry.size, ourEntry.realSize, decomp);
		mutex->unlock();
		delete[] outp;
		return decomp;
	}
	return NULL;
}

std::string CLodHandler::getFileName(std::string lodFile, LodFileType type)
{
	convertName(lodFile);
	boost::unordered_set<Entry>::const_iterator it = entries.find(Entry(lodFile, type));

	if (it != entries.end())
		return it->realName;
	return "";
}

bool CLodHandler::haveFile(std::string name, LodFileType type)
{
	convertName(name);
	return vstd::contains(entries, Entry(name, type));
}

DLL_EXPORT int CLodHandler::infs2(unsigned char * in, int size, int realSize, unsigned char *& out, int wBits)
{
	int ret;
	unsigned have;
	z_stream strm;
	out = new unsigned char [realSize];
	int latPosOut = 0;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, wBits);
	if (ret != Z_OK)
		return ret;
	int chunkNumber = 0;
	do
	{
		if(size < chunkNumber * NLoadHandlerHelp::fCHUNK)
			break;
		strm.avail_in = std::min(NLoadHandlerHelp::fCHUNK, size - chunkNumber * NLoadHandlerHelp::fCHUNK);
		if (strm.avail_in == 0)
			break;
		strm.next_in = in + chunkNumber * NLoadHandlerHelp::fCHUNK;

		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = realSize - latPosOut;
			strm.next_out = out + latPosOut;
			ret = inflate(&strm, Z_NO_FLUSH);
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			bool breakLoop = false;
			switch (ret)
			{
			case Z_STREAM_END:
				breakLoop = true;
				break;
			case Z_NEED_DICT:
				ret = Z_DATA_ERROR;	 /* and fall through */
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				(void)inflateEnd(&strm);
				return ret;
			}

			if(breakLoop)
				break;

			have = realSize - latPosOut - strm.avail_out;
			latPosOut += have;
		} while (strm.avail_out == 0);

		++chunkNumber;
		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void CLodHandler::extractFile(const std::string FName, const std::string name)
{
	int len; //length of file to write
	unsigned char * outp = giveFile(name, FILE_ANY, &len);
	std::ofstream out;
	out.open(FName.c_str(), std::ios::binary);
	if(!out.is_open())
	{
		tlog1<<"Unable to create "<<FName<<std::endl;
	}
	else
	{
		out.write(reinterpret_cast<char*>(outp), len);
		out.close();
	}
}

void CLodHandler::initEntry(Entry &e, std::string name)
{
	std::string ext;
	convertName(name, &ext);
	e.name = name;

	std::map<std::string, LodFileType>::iterator it = extMap.find(ext);
	if (it == extMap.end())
		e.type = FILE_OTHER;
	else
		e.type = it->second;
}

void CLodHandler::init(const std::string lodFile, const std::string dirName)
{
	#define EXT(NAME, TYPE) extMap.insert(std::pair<std::string, LodFileType>(NAME, TYPE));
	EXT(".TXT", FILE_TEXT);
	EXT(".JSON",FILE_TEXT);
	EXT(".DEF", FILE_ANIMATION);
	EXT(".MSK", FILE_MASK);
	EXT(".MSG", FILE_MASK);
	EXT(".H3C", FILE_CAMPAIGN);
	EXT(".H3M", FILE_MAP);
	EXT(".FNT", FILE_FONT);
	EXT(".BMP", FILE_GRAPHICS);
	EXT(".JPG", FILE_GRAPHICS);
	EXT(".PCX", FILE_GRAPHICS);
	EXT(".PNG", FILE_GRAPHICS);
	EXT(".TGA", FILE_GRAPHICS);
	#undef EXT
	
	myDir = dirName;
	
	LOD.open(lodFile.c_str(), std::ios::in | std::ios::binary);

	if (!LOD.is_open()) 
	{
		tlog1 << "Cannot open " << lodFile << std::endl;
		return;
	}

	Uint32 temp;
	LOD.seekg(8);
	LOD.read((char *)&temp, 4);
	totalFiles = SDL_SwapLE32(temp);

	LOD.seekg(0x5c, std::ios::beg);
	if(!LOD)
	{
		tlog2 << lodFile << " doesn't store anything!\n";
		return;
	}

	struct LodEntry *lodEntries = new struct LodEntry[totalFiles];
	LOD.read((char *)lodEntries, sizeof(struct LodEntry) * totalFiles);

	for (unsigned int i=0; i<totalFiles; i++)
	{
		Entry entry;
		initEntry(entry, lodEntries[i].filename);
		
		entry.offset= SDL_SwapLE32(lodEntries[i].offset);
		entry.realSize = SDL_SwapLE32(lodEntries[i].uncompressedSize);
		entry.size = SDL_SwapLE32(lodEntries[i].size);

		entries.insert(entry);
	}

	delete [] lodEntries;

	boost::filesystem::recursive_directory_iterator enddir;
	if(boost::filesystem::exists(dirName))
	{
		std::vector<std::string> path;
		for (boost::filesystem::recursive_directory_iterator dir(dirName); dir!=enddir; dir++)
		{
			//If a directory was found - add name to vector to recreate full path later
			if (boost::filesystem::is_directory(dir->status()))
			{
				path.resize(dir.level()+1);
				path.back() = dir->path().leaf();
			}
			if(boost::filesystem::is_regular(dir->status()))
			{
				Entry e;

				//we can't get relative path with boost at the moment - need to create path to file manually
				for (size_t i=0; i<dir.level() && i<path.size(); i++)
					e.realName += path[i] + '/';

				e.realName += dir->path().leaf();

				initEntry(e, e.realName);

				if(vstd::contains(entries, e)) //file present in .lod - overwrite its entry
					entries.erase(e);

				e.offset = -1;
				e.realSize = e.size = boost::filesystem::file_size(dir->path());
				entries.insert(e);
			}
		}
	}
	else
	{
		tlog1<<"Warning: No "+dirName+"/ folder!"<<std::endl;
	}
}
std::string CLodHandler::getTextFile(std::string name, LodFileType type)
{
	int length=-1;
	unsigned char* data = giveFile(name, type, &length);

	if (!data) {
		tlog1<<"Fatal error. Missing game file: " << name << ". Aborting!"<<std::endl;
		exit(1);
	}

	std::string ret(data, data+length);
	delete [] data;
	return ret;
}

CLodHandler::CLodHandler()
{
	mutex = new boost::mutex;
	totalFiles = 0;
}

CLodHandler::~CLodHandler()
{
	delete mutex;
}

unsigned char * CLodHandler::getUnpackedFile( const std::string & path, int * sizeOut )
{
	const int bufsize = 65536;
	int mapsize = 0;

	gzFile map = gzopen(path.c_str(), "rb");
	assert(map);
	std::vector<unsigned char *> mapstr;

	// Read a map by chunks
	// We could try to read the map size directly (cf RFC 1952) and then read
	// directly the whole map, but that would create more problems.
	do {
		unsigned char *buf = new unsigned char[bufsize];

		int ret = gzread(map, buf, bufsize);
		if (ret == 0 || ret == -1) {
			delete [] buf;
			break;
		}

		mapstr.push_back(buf);
		mapsize += ret;
	} while(1);

	gzclose(map);

	// Now that we know the uncompressed size, reassemble the chunks
	unsigned char *initTable = new unsigned char[mapsize];

	std::vector<unsigned char *>::iterator it;
	int offset;
	int tocopy = mapsize;
	for (it = mapstr.begin(), offset = 0; 
		it != mapstr.end(); 
		it++, offset+=bufsize ) {
			memcpy(&initTable[offset], *it, tocopy > bufsize ? bufsize : tocopy);
			tocopy -= bufsize;
			delete [] *it;
	}

	*sizeOut = mapsize;
	return initTable;
}
