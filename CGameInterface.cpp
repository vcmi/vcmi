#include "stdafx.h"
#include "CGameInterface.h"

#ifdef _WIN32
	#include <windows.h> //for .dll libs
#else
	#include <dlfcn.h>
#endif

/*
 * CGameInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CGlobalAI * CAIHandler::getNewAI(CCallback * cb, std::string dllname)
{
	char temp[50];
	CGlobalAI * ret=NULL;
	CGlobalAI*(*getAI)(); 
	void(*getName)(char*); 

	std::string dllPath;

#ifdef _WIN32
	dllPath = "AI/"+dllname+".dll";
	HINSTANCE dll = LoadLibraryA(dllPath.c_str());
	if (!dll)
	{
		tlog1 << "Cannot open AI library ("<<dllPath<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open AI library");
	}
	//int len = dllname.size()+1;
	getName = (void(*)(char*))GetProcAddress(dll,"GetAiName");
	getAI = (CGlobalAI*(*)())GetProcAddress(dll,"GetNewAI");
#else
	dllPath = "AI/"+dllname+".so";
	void *dll = dlopen(dllPath.c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (!dll)
	{
		tlog1 << "Cannot open AI library ("<<dllPath<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open AI library");
	}
	getName = (void(*)(char*))dlsym(dll,"GetAiName");
	getAI = (CGlobalAI*(*)())dlsym(dll,"GetNewAI");
#endif
	getName(temp);
	tlog0 << "Loaded AI named " << temp << std::endl;
	ret = getAI();

	if(!ret)
		tlog1 << "Cannot get AI!\n";

	ret->dllName = dllname;	 
	return ret;
}
