#include "StdInc.h"

#include "../../AI_Base.h"
#include "CEmptyAI.h"

std::set<CGlobalAI*> ais;
extern "C" DLL_LINKAGE int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_LINKAGE void GetAiName(char* name)
{
	strcpy(name,NAME);
}
extern "C" DLL_LINKAGE char * GetAiNameS()
{
	char * ret = new char[50];
	strcpy(ret,NAME);
	return ret;
}
extern "C" DLL_LINKAGE CGlobalAI * GetNewAI()
{
	return new CEmptyAI();
// return
}
extern "C" DLL_LINKAGE void ReleaseAI(CGlobalAI * i)
{
	delete (CEmptyAI*)i;
	ais.erase(i);
}
