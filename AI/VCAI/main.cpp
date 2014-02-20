#include "StdInc.h"
#include "VCAI.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

#ifdef __ANDROID__
#define GetGlobalAiVersion VCAI_GetGlobalAiVersion
#define GetAiName VCAI_GetAiName
#define GetNewAI VCAI_GetNewAI
#endif

static const char *g_cszAiName = "VCAI";

extern "C" DLL_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_EXPORT void GetNewAI(shared_ptr<CGlobalAI> &out)
{
	out = make_shared<VCAI>();
}
