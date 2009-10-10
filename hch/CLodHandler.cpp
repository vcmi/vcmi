#define VCMI_DLL
#include "../stdafx.h"
#include "zlib.h"
#include "CLodHandler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <fstream>
#include "boost/filesystem/operations.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>
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

DLL_EXPORT int readNormalNr (int pos, int bytCon, const unsigned char * str)
{
	int ret=0;
	int amp=1;
	if (str)
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=str[pos+i]*amp;
			amp<<=8;
		}
	}
	else return -1;
	return ret;
}

unsigned char * CLodHandler::giveFile(std::string defName, int * length)
{
	std::transform(defName.begin(), defName.end(), defName.begin(), (int(*)(int))toupper);
	Entry * ourEntry = entries.znajdz(Entry(defName));
	if(!ourEntry) //nothing's been found
	{
		tlog1 << "Cannot find file: " << defName << std::endl;
		return NULL;
	}
	if(length) *length = ourEntry->realSize;
	mutex->lock();

	unsigned char * outp;
	if (ourEntry->offset<0) //file is in the sprites/ folder; no compression
	{
		int result;
		unsigned char * outp = new unsigned char[ourEntry->realSize];
		FILE * f = fopen((myDir + "/" + ourEntry->nameStr).c_str(), "rb");
		if (f) {
			result = fread(outp,1,ourEntry->realSize,f);
			fclose(f);
		} else
			result = -1;
		mutex->unlock();
		if(result<0) {
			tlog1<<"Error in file reading: " << myDir << "/" << ourEntry->nameStr << std::endl;
			delete[] outp;
			return NULL;
		} else
			return outp;
	}
	else if (ourEntry->size==0) //file is not compressed
	{
		outp = new unsigned char[ourEntry->realSize];

		LOD.seekg(ourEntry->offset, std::ios::beg);
		LOD.read((char*)outp, ourEntry->realSize);
		mutex->unlock();
		return outp;
	}
	else //we will decompress file
	{
		outp = new unsigned char[ourEntry->size];

		LOD.seekg(ourEntry->offset, std::ios::beg);
		LOD.read((char*)outp, ourEntry->size);
		mutex->unlock();
		unsigned char * decomp = NULL;
		int decRes = infs2(outp, ourEntry->size, ourEntry->realSize, decomp);
		delete[] outp;
		return decomp;
	}
	return NULL;
}

int CLodHandler::infs(unsigned char * in, int size, int realSize, std::ofstream & out, int wBits)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char inx[NLoadHandlerHelp::fCHUNK];
	unsigned char outx[NLoadHandlerHelp::fCHUNK];

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
		int readBytes = 0;
		for(int i=0; i<NLoadHandlerHelp::fCHUNK && (chunkNumber * NLoadHandlerHelp::fCHUNK + i)<size; ++i)
		{
			inx[i] = in[chunkNumber * NLoadHandlerHelp::fCHUNK + i];
			++readBytes;
		}
		++chunkNumber;
		strm.avail_in = readBytes;
		//strm.avail_in = fread(inx, 1, NLoadHandlerHelp::fCHUNK, source);
		/*if (in.bad())
		{
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}*/
		if (strm.avail_in == 0)
			break;
		strm.next_in = inx;

		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = NLoadHandlerHelp::fCHUNK;
			strm.next_out = outx;
			ret = inflate(&strm, Z_NO_FLUSH);
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;	 /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
			have = NLoadHandlerHelp::fCHUNK - strm.avail_out;
			/*if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}*/
			out.write((char*)outx, have);
			if(out.bad())
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
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

void CLodHandler::extractFile(std::string FName, std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))toupper);
	for (int i=0;i<totalFiles;i++)
	{
		std::string buf1 = entries[i].nameStr;
		std::transform(buf1.begin(), buf1.end(), buf1.begin(), (int(*)(int))toupper);
		if(buf1!=name)
			continue;
		LOD.seekg(entries[i].offset, std::ios::beg);
		std::string bufff = (FName);
		unsigned char * outp;
		if (entries[i].size==0) //file is not compressed
		{
			outp = new unsigned char[entries[i].realSize];
			LOD.read((char*)outp, entries[i].realSize);
			std::ofstream out;
			out.open(bufff.c_str(), std::ios::binary);
			if(!out.is_open())
			{
				tlog1<<"Unable to create "<<bufff;
			}
			else
			{
				for(int hh=0; hh<entries[i].realSize; ++hh)
				{
					out<<*(outp+hh);
				}
				out.close();
			}
		}
		else //we will decompressing file
		{
			outp = new unsigned char[entries[i].size];
			LOD.read((char*)outp, entries[i].size);
			std::ofstream destin;
			destin.open(bufff.c_str(), std::ios::binary);
			//int decRes = decompress(outp, entries[i].size, entries[i].realSize, bufff);
			int decRes = infs(outp, entries[i].size, entries[i].realSize, destin);
			destin.close();
			if(decRes!=0)
			{
				tlog1<<"LOD Extraction error"<<"  "<<decRes<<" while extracting to "<<bufff<<std::endl;
			}
		}
		delete[] outp;
	}
}

int CLodHandler::readNormalNr (const unsigned char* bufor, int bytCon, bool cyclic)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[i]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}

void CLodHandler::init(std::string lodFile, std::string dirName)
{
	myDir = dirName;
	std::string Ts;
	Uint32 temp;

	LOD.open(lodFile.c_str(), std::ios::in | std::ios::binary);

	if (!LOD.is_open()) {
		tlog1 << "Cannot open " << lodFile << std::endl;
		return;
	}

	LOD.seekg(8);
	LOD.read((char *)&temp, 4);
	totalFiles = SDL_SwapLE32(temp);

	LOD.seekg(0x5c, std::ios::beg);

	struct LodEntry *lodEntries = new struct LodEntry[totalFiles];
	LOD.read((char *)lodEntries, sizeof(struct LodEntry) * totalFiles);

	for (unsigned int i=0; i<totalFiles; i++)
	{
		Entry entry;

		entry.nameStr = lodEntries[i].filename;
		std::transform(entry.nameStr.begin(), entry.nameStr.end(), 
					   entry.nameStr.begin(), toupper);

		entry.offset= SDL_SwapLE32(lodEntries[i].offset);
		entry.realSize = SDL_SwapLE32(lodEntries[i].uncompressedSize);
		entry.size = SDL_SwapLE32(lodEntries[i].size);

		entries.push_back(entry);
	}

	delete [] lodEntries;

	boost::filesystem::directory_iterator enddir;
	if(boost::filesystem::exists(dirName))
	{
		for (boost::filesystem::directory_iterator dir(dirName);dir!=enddir;dir++)
		{
			if(boost::filesystem::is_regular(dir->status()))
			{
				std::string name = dir->path().leaf();
				std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))toupper);
				boost::algorithm::replace_all(name,".BMP",".PCX");
				Entry * e = entries.znajdz(name);
				if(e) //file present in .lod - overwrite its entry
				{
					e->offset = -1;
					e->realSize = e->size = boost::filesystem::file_size(dir->path());
				}
				else //file not present in lod - add entry for it
				{
					Entry e2;
					e2.offset = -1;
					e2.nameStr = name;
					e2.realSize = e2.size = boost::filesystem::file_size(dir->path());
					entries.push_back(e2);
				}
			}
		}
	}
	else
	{
		tlog1<<"Warning: No "+dirName+"/ folder!"<<std::endl;
	}
}
std::string CLodHandler::getTextFile(std::string name)
{
	int length=-1;
	unsigned char* data = giveFile(name,&length);

	if (!data) {
		tlog1<<"Fatal error. Missing game file. Aborting!"<<std::endl;
		exit(1);
	}

	std::string ret;
	ret.reserve(length);
	for(int i=0;i<length;i++)
		ret+=data[i];
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
