#include "StdInc.h"

#include "../../lib/AI_Base.h"
#include "ObjectVisitingModule.h"

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

extern "C" DLL_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, 100, "ObjectVisitingModule");
}

extern "C" DLL_EXPORT void GetNewAutomationModule(shared_ptr<ObjectVisitingModule> &out)
{
	out = make_shared<ObjectVisitingModule>();
}