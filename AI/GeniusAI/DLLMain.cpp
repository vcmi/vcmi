#include "StdInc.h"
#include "../../lib/AI_Base.h"
#include "CGeniusAI.h"

using namespace geniusai;

const char *g_cszAiName = "Genius 1.0";

extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT char* GetAiNameS()
{
	// need to be defined
	return NULL;
}

extern "C" DLL_EXPORT CGlobalAI* GetNewAI()
{
	return new CGeniusAI();
}

extern "C" DLL_EXPORT void ReleaseAI(CGlobalAI* i)
{
	delete (CGeniusAI*)i;
}

extern "C" DLL_EXPORT CBattleGameInterface* GetNewBattleAI()
{
	return new CGeniusAI();
}