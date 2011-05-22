#define VCMI_DLL

#include "ERMScriptModule.h"
#include "ERMInterpreter.h"
#include "CObjectHandler.h"

IGameEventRealizer *acb;
CPrivilagedInfoCallback *icb;


CScriptingModule * getERMModule()
{
	CScriptingModule *ret = new ERMInterpreter();
	return ret;
}

CScriptingModule::~CScriptingModule()
{

}
