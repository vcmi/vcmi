#include "stdafx.h"
#include "CGameInterface.h"
#include "lib/BattleState.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN //excludes rarely used stuff from windows headers - delete this line if something is missing
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

template<typename rett>
rett * createAny(std::string dllname, std::string methodName)
{
	char temp[50];
	rett * ret=NULL;
	rett*(*getAI)(); 
	void(*getName)(char*); 

	std::string dllPath;

	//TODO unify at least partially (code duplication)
#ifdef _WIN32
	dllPath = dllname;
	HINSTANCE dll = LoadLibraryA(dllPath.c_str());
	if (!dll)
	{
		tlog1 << "Cannot open dynamic library ("<<dllPath<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open dynamic library");
	}
	//int len = dllname.size()+1;
	getName = (void(*)(char*))GetProcAddress(dll,"GetAiName");
	getAI = (rett*(*)())GetProcAddress(dll,methodName.c_str());
#else
	dllPath = dllname;
	void *dll = dlopen(dllPath.c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (!dll)
	{
		tlog1 << "Cannot open dynamic library ("<<dllPath<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open dynamic library");
	}
	getName = (void(*)(char*))dlsym(dll,"GetAiName");
	getAI = (rett*(*)())dlsym(dll,methodName.c_str());
#endif
	getName(temp);
	tlog0 << "Loaded AI named " << temp << std::endl;
	ret = getAI();

	if(!ret)
		tlog1 << "Cannot get AI!\n";

	return ret;
}


template<typename rett>
rett * createAnyAI(std::string dllname, std::string methodName)
{
	rett* ret = createAny<rett>(LIB_DIR "/" + dllname + '.' + LIB_EXT, methodName);
	ret->dllName = dllname;	
	return ret;
}

CGlobalAI * CDynLibHandler::getNewAI(std::string dllname)
{
	return createAnyAI<CGlobalAI>(dllname, "GetNewAI");
}

CBattleGameInterface * CDynLibHandler::getNewBattleAI(std::string dllname )
{
	return createAnyAI<CBattleGameInterface>(dllname, "GetNewBattleAI");
}

CScriptingModule * CDynLibHandler::getNewScriptingModule(std::string dllname)
{
	return createAny<CScriptingModule>(dllname, "GetNewModule");
}

BattleAction CGlobalAI::activeStack( const CStack * stack )
{
	BattleAction ba; ba.actionType = BattleAction::DEFEND;
	ba.stackNumber = stack->ID;
	return ba;
}
