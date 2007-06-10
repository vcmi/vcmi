#include "stdafx.h"
#include "CSemiLodHandler.h"
void CSemiLodHandler::openLod(std::string path)
{
	ourName = path;
};
CSemiDefHandler * CSemiLodHandler::giveDef(std::string name)
{
	CSemiDefHandler * ret = new CSemiDefHandler();
	ret->openDef(name, ourName);
	return ret;
};