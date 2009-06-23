#ifndef __CLODHANDLER_H__
#define __CLODHANDLER_H__
#include "../global.h"
#include <fstream>
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
{class mutex;};
namespace NLoadHandlerHelp
{
	const int dmHelp=0, dmNoExtractingMask=1;
	//std::string P1,P2,CurDir;
	const int fCHUNK = 50000;
};

struct Entry
{
	unsigned char name[12], //filename
		hlam_1[4], //???
		hlam_2[4]; //probably type of file
	std::string nameStr;
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
public:
	FILE* FLOD;
	nodrze<Entry> entries;
	unsigned int totalFiles;
	boost::mutex *mutex;
	std::string myDir; //load files from this dir instead of .lod file

	CLodHandler();
	~CLodHandler();
	int readNormalNr (unsigned char* bufor, int bytCon, bool cyclic=false); //lod header reading helper
	int infs(unsigned char * in, int size, int realSize, std::ofstream & out, int wBits=15); //zlib fast handler
	int infs2(unsigned char * in, int size, int realSize, unsigned char*& out, int wBits=15); //zlib fast handler
	unsigned char * giveFile(std::string defName, int * length=NULL); //returns pointer to the decompressed data - it must be deleted when no longer needed!
	std::string getTextFile(std::string name); //extracts one file
	void extract(std::string FName);
	void extractFile(std::string FName, std::string name); //extracts a specific file
	void init(std::string lodFile, std::string dirName);
};


#endif // __CLODHANDLER_H__
