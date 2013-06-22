#include "StdInc.h"

#include "../../lib/AI_Base.h"
#include "BattleAI.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

const char *g_cszAiName = "Battle AI";

extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewBattleAI(shared_ptr<CBattleGameInterface> &out)
{
	out = make_shared<CBattleAI>();
}