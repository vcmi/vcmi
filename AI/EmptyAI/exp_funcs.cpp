#include "../../AI_Base.h"
#include "CEmptyAI.h"
#include <cstring>
#include <set>
std::set<CAIBase*> ais;

DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,NAME);
}
DLL_EXPORT CAIBase * GetNewAI()
{
	return new CEmptyAI();
// return
}
DLL_EXPORT void ReleaseAI(CAIBase * i)
{
	//delete (TTAICore*)i;
	//ais.erase(i);
}