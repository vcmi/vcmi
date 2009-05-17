#define VCMI_DLL
#include "../stdafx.h"
#include "zlib.h"
#include "CLodHandler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include "boost/filesystem/operations.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/thread.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
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

DLL_EXPORT int readNormalNr (int pos, int bytCon, unsigned char * str)
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

const unsigned char *CLodHandler::dataptr()
{ 
	return (const unsigned char *)mmlod->data();
}

unsigned char * CLodHandler::giveFile(std::string defName, int * length)
{
	std::transform(defName.begin(), defName.end(), defName.begin(), (int(*)(int))toupper);
	Entry * ourEntry = entries.znajdz(Entry(defName));
	if(!ourEntry) //nothing's been found
	{
		tlog1<<"Cannot find file: "<<defName;
		return NULL;
	}
	if(length) *length = ourEntry->realSize;

	const unsigned char *data = dataptr();
	data += ourEntry->offset;

	unsigned char * outp;
	if (ourEntry->offset<0) //file is in the sprites/ folder; no compression
	{
		unsigned char * outp = new unsigned char[ourEntry->realSize];
		char name[30];

		memset(name,0, sizeof(name));
		strcat(name, myDir.c_str());
		strcat(name, PATHSEPARATOR);
		strcat(name,ourEntry->nameStr.c_str());
		FILE * f = fopen(name,"rb");
		int result = fread(outp,1,ourEntry->realSize,f);

		if (result<0) {
			tlog1<<"Error in file reading: " << name << std::endl;
			delete[] outp;
			return NULL;
		}
		else
			return outp;
	}

	else if (ourEntry->size==0)
	{
		//file is not compressed
		// TODO: we could try to avoid a copy here.
		outp = new unsigned char[ourEntry->realSize];
		memcpy(outp, data, ourEntry->realSize);
		return outp;
	}
	else
	{
		//we will decompress file
		unsigned char * decomp = NULL;
		int decRes = infs2(data, ourEntry->size, ourEntry->realSize, decomp);
		return decomp;
	}
	return NULL;
}
int CLodHandler::infs(const unsigned char * in, int size, int realSize, std::ofstream & out, int wBits)
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

DLL_EXPORT int CLodHandler::infs2(const unsigned char * in, int size, int realSize, unsigned char *& out, int wBits)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char inx[NLoadHandlerHelp::fCHUNK];
	unsigned char outx[NLoadHandlerHelp::fCHUNK];
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
		int readBytes = 0;
		for(int i=0; i<NLoadHandlerHelp::fCHUNK && (chunkNumber * NLoadHandlerHelp::fCHUNK + i)<size; ++i)
		{
			inx[i] = in[chunkNumber * NLoadHandlerHelp::fCHUNK + i];
			++readBytes;
		}
		++chunkNumber;
		strm.avail_in = readBytes;
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
			for(int oo=0; oo<have; ++oo)
			{
				out[latPosOut] = outx[oo];
				++latPosOut;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void CLodHandler::extract(std::string FName)
{
	std::ofstream FOut;
	for (int i=0;i<totalFiles;i++)
	{
		const unsigned char *data = dataptr();
		data += entries[i].offset;

		std::string bufff = (DATA_DIR + FName.substr(0, FName.size()-4) + PATHSEPARATOR + entries[i].nameStr);
		unsigned char * outp;
		if (entries[i].size==0) //file is not compressed
		{
			// TODO: this could be better written
			outp = new unsigned char[entries[i].realSize];
			memcpy(outp, data, entries[i].realSize);
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
		else
		{
			// TODO: this could be better written
			outp = new unsigned char[entries[i].size];
			memcpy(outp, data, entries[i].size);
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

void CLodHandler::extractFile(std::string FName, std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))toupper);
	for (int i=0;i<totalFiles;i++)
	{
		std::string buf1 = entries[i].nameStr;
		std::transform(buf1.begin(), buf1.end(), buf1.begin(), (int(*)(int))toupper);
		if(buf1!=name)
			continue;

		const unsigned char *data = dataptr();
		data += entries[i].offset;

		std::string bufff = (FName);
		unsigned char * outp;
		if (entries[i].size==0) //file is not compressed
		{
			// TODO: this could be better written
			outp = new unsigned char[entries[i].realSize];
			memcpy(outp, data, entries[i].realSize);
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
			// TODO: this could be better written
			outp = new unsigned char[entries[i].size];
			memcpy(outp, data, entries[i].size);
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

void CLodHandler::init(const std::string lodFile, const std::string dirName)
{
	myDir = dirName;
	std::string Ts;

	mmlod = new boost::iostreams::mapped_file_source(lodFile);
	if(!mmlod->is_open())
	{
		delete mmlod;
		mmlod = NULL;
		tlog1 << "Cannot open " << lodFile << std::endl;
		return;
	}

	const unsigned char *data = dataptr();

	totalFiles = readNormalNr(&data[8], 4);
	data += 0x5c;

	for (unsigned int i=0; i<totalFiles; i++)
	{
		Entry entry;

		for(int kk=0; kk<12; ++kk)
        {
            if (data[kk] !=0 )
				entry.nameStr+=toupper(data[kk]);
            else
				break;
        }
		data += 16;

		entry.offset=readNormalNr(data, 4);
		data += 4;

		entry.realSize=readNormalNr(data, 4);
		data += 8;

		entry.size=readNormalNr(data, 4);
		data += 4;

		entries.push_back(entry);
	}
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

	std::string ret;
	ret.reserve(length);
	for(int i=0;i<length;i++)
		ret+=data[i];
	delete [] data;
	return ret;
}
