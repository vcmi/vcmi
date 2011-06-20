#include "ERMScriptModule.h"
#include "ERMInterpreter.h"

IGameEventRealizer *acb;
CPrivilagedInfoCallback *icb;


#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

const char *g_cszAiName = "(V)ERM interpreter";

extern "C" DLL_F_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(g_cszAiName) + 1, g_cszAiName);
}

extern "C" DLL_F_EXPORT CScriptingModule* GetNewModule()
{
	return new ERMInterpreter();
}