#include "stdafx.h"
#include "StupidAI.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

const char *g_cszAiName = "Stupid AI 0.1";

extern "C" DLL_F_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_F_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_F_EXPORT char* GetAiNameS()
{
	// need to be defined
	return NULL;
}

extern "C" DLL_F_EXPORT CBattleGameInterface* GetNewBattleAI()
{
	return new CStupidAI();
}

extern "C" DLL_F_EXPORT void ReleaseBattleAI(CBattleGameInterface* i)
{
	delete (CStupidAI*)i;
}
