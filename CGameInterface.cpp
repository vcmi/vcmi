#include "stdafx.h"
#include "CGameInterface.h"

#ifdef _WIN32
	#include <windows.h> //for .dll libs
#else
	#include <dlfcn.h>
#endif

CGlobalAI * CAIHandler::getNewAI(CCallback * cb, std::string dllname)
{
	char temp[50];
	dllname = "AI/"+dllname;
	CGlobalAI * ret=NULL;
	CGlobalAI*(*getAI)(); 
	void(*getName)(char*); 

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
	if (!dll)
	{
		tlog1 << "Cannot open AI library ("<<dllname<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open AI library");
	}
	getName = (void(*)(char*))dlsym(dll,"GetAiName");
	getAI = (CGlobalAI*(*)())dlsym(dll,"GetNewAI");
#endif
	getName(temp);
	tlog0 << "Loaded .dll with AI named " << temp << std::endl;
	ret = getAI();

	if(!ret)
		tlog1 << "Cannot get AI!\n";

	ret->dllName = dllname;	 
	return ret;
}
