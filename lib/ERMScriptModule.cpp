#define VCMI_DLL

#include "ERMScriptModule.h"
#include "ERMInterpreter.h"
#include <boost/assign/std/vector.hpp> 
#include <boost/assign/list_of.hpp>
#include "CObjectHandler.h"

using namespace boost::assign;

CScriptingModule::~CScriptingModule()
{

}

CERMScriptModule::CERMScriptModule(void)
{
}

CERMScriptModule::~CERMScriptModule(void)
{
}

void CERMScriptModule::init()
{
	interpreter = new ERMInterpreter();
	interpreter->init();
	interpreter->scanForScripts();
	interpreter->scanScripts();
	interpreter->executeInstructions();
	interpreter->executeTriggerType("PI");
}

void CERMScriptModule::heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start)
{
	if(!visitedObj)
		return;

	ERMInterpreter::TIDPattern tip;
	tip[1] = list_of(visitedObj->ID);
	tip[2] = list_of(visitedObj->ID)(visitedObj->subID);
	tip[3] = list_of(visitedObj->pos.x)(visitedObj->pos.y)(visitedObj->pos.z);
	interpreter->executeTriggerType(VERMInterpreter::TriggerType("OB"), start, tip);
}

void CERMScriptModule::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	interpreter->executeTriggerType("BA", 0);
	interpreter->executeTriggerType("BR", -1);
	interpreter->executeTriggerType("BF", 0);
	//TODO tactics or not
}