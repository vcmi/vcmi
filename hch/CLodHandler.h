#ifndef CLODHANDLER_H
#define CLODHANDLER_H
#include "../global.h"
#include <fstream>
#include <vector>
#include <string>
#include "../nodrze.h"

struct SDL_Surface;
class CDefHandler;
class CDefEssential;

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
 class CLodHandler
{
public:
	FILE* FLOD;
	nodrze<Entry> entries;
	unsigned int totalFiles;

	DLL_EXPORT int readNormalNr (unsigned char* bufor, int bytCon, bool cyclic=false); //lod header reading helper
	DLL_EXPORT int infs(unsigned char * in, int size, int realSize, std::ofstream & out, int wBits=15); //zlib fast handler
	DLL_EXPORT int infs2(unsigned char * in, int size, int realSize, unsigned char*& out, int wBits=15); //zlib fast handler
	DLL_EXPORT unsigned char * giveFile(std::string defName, int * length=NULL); //returns pointer to the decompressed data - it must be deleted when no longer needed!
	DLL_EXPORT std::string getTextFile(std::string name); //extracts one file
	DLL_EXPORT void extract(std::string FName);
	DLL_EXPORT void extractFile(std::string FName, std::string name); //extracts a specific file
	DLL_EXPORT void init(std::string lodFile);
};

#endif //CLODHANDLER_H