#define VCMI_DLL
#include "../../AI_Base.h"
#include "CEmptyAI.h"
#include <cstring>
#include <set>
std::set<CGlobalAI*> ais;
extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,NAME);
}
extern "C" DLL_EXPORT char * GetAiNameS()
{
	char * ret = new char[50];
	strcpy(ret,NAME);
	return ret;
}
extern "C" DLL_EXPORT CGlobalAI * GetNewAI()
{
	return new CEmptyAI();
// return
}
extern "C" DLL_EXPORT void ReleaseAI(CGlobalAI * i)
{
	delete (CEmptyAI*)i;
	ais.erase(i);
}