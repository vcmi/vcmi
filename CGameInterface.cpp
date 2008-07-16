	#include "stdafx.h"
#include "CGameInterface.h"
#include "CAdvMapInterface.h"
#include "CMessage.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
#include "CScreenHandler.h"
#include "CCursorHandler.h"
#include "CCallback.h"
#include "SDL_Extensions.h"
#include "hch/CLodHandler.h"
#include "CPathfinder.h"
#include <sstream>
#include "hch/CHeroHandler.h"
#include "SDL_framerate.h"
#include "AI/EmptyAI/CEmptyAI.h"

#ifdef _WIN32
	#include <windows.h> //for .dll libs
#else
	#include <dlfcn.h>
#endif
using namespace CSDL_Ext;

CGlobalAI * CAIHandler::getNewAI(CCallback * cb, std::string dllname)
{
	dllname = "AI/"+dllname;
	CGlobalAI * ret=NULL;
	CGlobalAI*(*getAI)();
	void(*getName)(char*);

#ifdef _WIN32
	HINSTANCE dll = LoadLibraryA(dllname.c_str());
	if (!dll)
	{
		std::cout << "Cannot open AI library ("<<dllname<<"). Throwing..."<<std::endl;
		throw new std::exception("Cannot open AI library");
	}
	//int len = dllname.size()+1;
	getName = (void(*)(char*))GetProcAddress(dll,"GetAiName");
	getAI = (CGlobalAI*(*)())GetProcAddress(dll,"GetNewAI");
#else
	; //TODO: handle AI library on Linux
#endif
	char * temp = new char[50];
#ifndef __amigaos4__
	getName(temp);
#endif
	std::cout << "Loaded .dll with AI named " << temp << std::endl;
	delete temp;
#ifndef __amigaos4__
	ret = getAI();
	ret->init(cb);
#else
	//ret = new CEmptyAI();
#endif
	return ret;
}
//CGlobalAI::CGlobalAI()
//{
//}
