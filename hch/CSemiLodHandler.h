#ifndef CSEMILODHANDLER_H
#define CSEMILODHANDLER_H

#include "CSemiDefHandler.h"
class CSemiLodHandler
{
public:
	std::string ourName; // name of our lod
	void openLod(std::string path);
	CSemiDefHandler * giveDef(std::string name, int dist=1); //loads def from our lod
};

#endif //CSEMILODHANDLER_H