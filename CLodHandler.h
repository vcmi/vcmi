#ifndef CLODHANDLER_H
#define CLODHANDLER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "zlib.h"

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
	int offset, //from beginning
		realSize, //size without compression
		size;	//and with
};

class CLodHandler
{
public:
	std::vector<Entry> entries;
	unsigned int totalFiles;

	int readNormalNr (unsigned char* bufor, int bytCon, bool cyclic=false); //lod header reading helper
	int decompress (unsigned char * source, int size, int realSize, std::ofstream & dest); //main decompression function
	int decompress (unsigned char * source, int size, int realSize, std::string & dest); //main decompression function
	int infm(FILE *source, FILE *dest, int wBits = 15); //zlib handler
	void extract(std::string FName);
	void init(std::string lodFile);
};


#endif //CLODHANDLER_H