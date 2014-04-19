/*
 * IGameCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "IGameCallback.h"

/*#include "CGameState.h"
#include "mapping/CMap.h"
#include "CObjectHandler.h"
#include "CHeroHandler.h"
#include "StartInfo.h"
#include "CArtHandler.h"
#include "CSpellHandler.h"
#include "VCMI_Lib.h"
#include "CTownHandler.h"
#include "BattleState.h"*/
#include "NetPacks.h"
/*#include "CBuildingHandler.h"
#include "GameConstants.h"
#include "CModHandler.h"
#include "CDefObjInfoHandler.h"
#include "CBonusTypeHandler.h"

#include "Connection.h"*/

//TODO make clean
/*#define ERROR_SILENT_RET_VAL_IF(cond, txt, retVal) do {if(cond){return retVal;}} while(0)
#define ERROR_VERBOSE_OR_NOT_RET_VAL_IF(cond, verbose, txt, retVal) do {if(cond){if(verbose)logGlobal->errorStream() << BOOST_CURRENT_FUNCTION << ": " << txt; return retVal;}} while(0)
#define ERROR_RET_IF(cond, txt) do {if(cond){logGlobal->errorStream() << BOOST_CURRENT_FUNCTION << ": " << txt; return;}} while(0)
#define ERROR_RET_VAL_IF(cond, txt, retVal) do {if(cond){logGlobal->errorStream() << BOOST_CURRENT_FUNCTION << ": " << txt; return retVal;}} while(0)*/

const CGObjectInstance * IGameCallback::putNewObject(Obj ID, int subID, int3 pos)
{
	NewObject no;
	no.ID = ID; //creature
	no.subID= subID;
	no.pos = pos;
	commitPackage(&no);
	return getObj(no.id); //id field will be filled during applying on gs
}

const CGCreature * IGameCallback::putNewMonster(CreatureID creID, int count, int3 pos)
{
	const CGObjectInstance *m = putNewObject(Obj::MONSTER, creID, pos);
	setObjProperty(m->id, ObjProperty::MONSTER_COUNT, count);
	setObjProperty(m->id, ObjProperty::MONSTER_POWER, (si64)1000*count);
	return dynamic_cast<const CGCreature*>(m);
}

bool IGameCallback::isVisitCoveredByAnotherQuery(const CGObjectInstance *obj, const CGHeroInstance *hero)
{
	//only server knows
	assert(0);
	return false;
}
