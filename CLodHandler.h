#ifndef CLODHANDLER_H
#define CLODHANDLER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "zlib.h"
#include "CDefHandler.h"

enum Epcxformat {PCX8B, PCX24B};

namespace NLoadHandlerHelp
{
	const int dmHelp=0, dmNoExtractingMask=1;
	//std::string P1,P2,CurDir;
	const int fCHUNK = 50000;
};

struct Entry
{
	unsigned char name[12], //filename
		hlam_1[4], //
		hlam_2[4]; //
	std::string nameStr;
	int offset, //from beginning
		realSize, //size without compression
		size;	//and with
	bool operator<(const Entry & comp) const
	{
		return this->nameStr<comp.nameStr;
	}
	Entry(std::string con): nameStr(con){};
	Entry(){};
};
class CPCXConv
{	
public:
	unsigned char * pcx, *bmp;
	int pcxs, bmps;
	void fromFile(std::string path);
	void saveBMP(std::string path);
	void openPCX(char * PCX, int len);
	void openPCX();
	void convert();
	SDL_Surface * getSurface();
	CPCXConv(){pcx=bmp=NULL;pcxs=bmps=0;};
	~CPCXConv(){if (pcxs) delete pcx; if(bmps) delete bmp;}
};
class CLodHandler
{
private:
	std::ifstream FLOD;
public:
	std::vector<Entry> entries;
	unsigned int totalFiles;

	int readNormalNr (unsigned char* bufor, int bytCon, bool cyclic=false); //lod header reading helper
	int decompress (unsigned char * source, int size, int realSize, std::ofstream & dest); //main decompression function
	int decompress (unsigned char * source, int size, int realSize, std::string & dest); //main decompression function
	int infm(FILE *source, FILE *dest, int wBits = 15); //zlib handler
	int infs(unsigned char * in, int size, int realSize, std::ofstream & out, int wBits=15); //zlib fast handler
	int infs2(unsigned char * in, int size, int realSize, unsigned char*& out, int wBits=15); //zlib fast handler
	std::vector<CDefHandler *> extractManyFiles(std::vector<std::string> defNamesIn, std::string lodName); //extrats given files (defs only)
	void extract(std::string FName);
	void extractFile(std::string FName, std::string name); //extracts a specific file
	void init(std::string lodFile);
};


#endif //CLODHANDLER_H