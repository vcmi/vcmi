#include "../../AI_Base.h"
#include "CEmptyAI.h"
#include <cstring>
#include <set>
std::set<CGlobalAI*> ais;

DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,NAME);
}
DLL_EXPORT char * GetAiNameS()
{
	char * ret = new char[50];
	strcpy(ret,NAME);
	return ret;
}
DLL_EXPORT CGlobalAI * GetNewAI()
{
	return new CEmptyAI();
// return
}
DLL_EXPORT void ReleaseAI(CGlobalAI * i)
{
	delete (CEmptyAI*)i;
	ais.erase(i);
}