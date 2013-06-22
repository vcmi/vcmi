#include "StdInc.h"

#include "CEmptyAI.h"

std::set<CGlobalAI*> ais;
extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy(name,NAME);
}

extern "C" DLL_EXPORT void GetNewAI(shared_ptr<CGlobalAI> &out)
{
	out = make_shared<CEmptyAI>();
}