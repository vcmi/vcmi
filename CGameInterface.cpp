#include "stdafx.h"
#include "CGameInterface.h"
#include "CAdvmapInterface.h"
#include "CMessage.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "SDL_framerate.h"
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
	char temp[50];
	dllname = "AI/"+dllname;
	CGlobalAI * ret=NULL;
	CGlobalAI*(*getAI)(); //TODO use me
	void(*getName)(char*); //TODO use me

#ifdef _WIN32
	HINSTANCE dll = LoadLibraryA(dllname.c_str());
	if (!dll)
	{
		tlog1 << "Cannot open AI library ("<<dllname<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open AI library");
	}
	//int len = dllname.size()+1;
	getName = (void(*)(char*))GetProcAddress(dll,"GetAiName");
	getAI = (CGlobalAI*(*)())GetProcAddress(dll,"GetNewAI");
#else
	void *dll = dlopen(dllname.c_str(), RTLD_LOCAL | RTLD_LAZY);
	getName = (void(*)(char*))dlsym(dll,"GetAiName");
	getAI = (CGlobalAI*(*)())dlsym(dll,"GetNewAI");
	; //TODO: handle AI library on Linux
#endif
	getName(temp);
	tlog0 << "Loaded .dll with AI named " << temp << std::endl;
	ret = getAI();
	 
	return ret;
}
//CGlobalAI::CGlobalAI()
//{
//}
