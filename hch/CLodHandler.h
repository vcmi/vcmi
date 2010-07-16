#ifndef __CLODHANDLER_H__
#define __CLODHANDLER_H__
#include "../global.h"
#include <vector>
#include <string>
#include "../nodrze.h"

/*
 * CLodhandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct SDL_Surface;
class CDefHandler;
class CDefEssential;
namespace boost
{class mutex;}
namespace NLoadHandlerHelp
{
	const int dmHelp=0, dmNoExtractingMask=1;
	//std::string P1,P2,CurDir;
	const int fCHUNK = 50000;
}

struct LodEntry {
	char filename[16];
	ui32 offset;				/* little endian */
	ui32 uncompressedSize;	/* little endian */
	ui32 unused;				/* little endian */
	ui32 size;				/* little endian */
};

DLL_EXPORT int readNormalNr (const unsigned char * bufor, int pos, int bytCon = 4, bool cyclic = false);

DLL_EXPORT char readChar(const unsigned char * bufor, int &i);

DLL_EXPORT std::string readString(const unsigned char * bufor, int &i);

struct Entry
{
	// Info extracted from LOD file
	std::string nameStr,
		    realName;
	int offset, //from beginning
		realSize, //size without compression
		size;	//and with

	bool operator<(const std::string & comp) const
	{
		return nameStr<comp;
	}
	bool operator<(const Entry & comp) const
	{
		return nameStr<comp.nameStr;
	}
	Entry(std::string con): nameStr(con){};
	//Entry(unsigned char ): nameStr(con){};
	Entry(){};
};

class DLL_EXPORT CLodHandler
{
	std::ifstream LOD;
	unsigned int totalFiles;
	boost::mutex *mutex;
	std::string myDir; //load files from this dir instead of .lod file

public:
	nodrze<Entry> entries;

	CLodHandler();
	~CLodHandler();
	int infs2(unsigned char * in, int size, int realSize, unsigned char*& out, int wBits=15); //zlib fast handler
	unsigned char * giveFile(std::string defName, int * length=NULL); //returns pointer to the decompressed data - it must be deleted when no longer needed!
	std::string getTextFile(std::string name); //extracts one file
	void extractFile(std::string FName, std::string name); //extracts a specific file
	void init(std::string lodFile, std::string dirName);

	static unsigned char * getUnpackedFile(const std::string & path, int * sizeOut); //loads given file, decompresses and returns
};


#endif // __CLODHANDLER_H__
