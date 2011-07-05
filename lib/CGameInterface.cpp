#define VCMI_DLL
#include "CGameInterface.h"
#include "BattleState.h"

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN //excludes rarely used stuff from windows headers - delete this line if something is missing
	#include <windows.h> //for .dll libs
#else
	#include <dlfcn.h>
#endif

/*
 * CGameInterface.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template<typename rett>
rett * createAny(std::string dllname, std::string methodName)
{
	char temp[50];
	rett * ret=NULL;
	rett*(*getAI)(); 
	void(*getName)(char*); 

#ifdef _WIN32
	HINSTANCE dll = LoadLibraryA(dllname.c_str());
	if (dll)
	{
		getName = (void(*)(char*))GetProcAddress(dll,"GetAiName");
		getAI = (rett*(*)())GetProcAddress(dll,methodName.c_str());
	}
#else
	void *dll = dlopen(dllname.c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (dll)
	{
		getName = (void(*)(char*))dlsym(dll,"GetAiName");
		getAI = (rett*(*)())dlsym(dll,methodName.c_str());
	}
#endif
	if (!dll)
	{
		tlog1 << "Cannot open dynamic library ("<<dllname<<"). Throwing..."<<std::endl;
		throw new std::string("Cannot open dynamic library");
	}

	getName(temp);
	tlog0 << "Loaded " << temp << std::endl;
	ret = getAI();

	if(!ret)
		tlog1 << "Cannot get AI!\n";

	return ret;
}

//Currently AI libraries use "lib" prefix only on non-win systems.
//May be applied to Win systems as well to remove this ifdef
#ifdef _WIN32
std::string getAIFileName(std::string input)
{
	return input + '.' + LIB_EXT;
}
#else
std::string getAIFileName(std::string input)
{
	return "lib" + input + '.' + LIB_EXT;
}
#endif

template<typename rett>
rett * createAnyAI(std::string dllname, std::string methodName)
{
	std::string filename = getAIFileName(dllname);
	rett* ret = createAny<rett>(LIB_DIR "/AI/" + filename, methodName);
	ret->dllName = filename;
	return ret;
}

CGlobalAI * CDynLibHandler::getNewAI(std::string dllname)
{
	return createAnyAI<CGlobalAI>(dllname, "GetNewAI");
}

CBattleGameInterface * CDynLibHandler::getNewBattleAI(std::string dllname )
{
	return createAnyAI<CBattleGameInterface>(dllname, "GetNewBattleAI");
}

CScriptingModule * CDynLibHandler::getNewScriptingModule(std::string dllname)
{
	return createAny<CScriptingModule>(dllname, "GetNewModule");
}

BattleAction CGlobalAI::activeStack( const CStack * stack )
{
	BattleAction ba; ba.actionType = BattleAction::DEFEND;
	ba.stackNumber = stack->ID;
	return ba;
}

CGlobalAI::CGlobalAI()
{
	human = false;
}

void CAdventureAI::battleNewRound(int round)
{
	battleAI->battleNewRound(round);
}

void CAdventureAI::battleCatapultAttacked(const CatapultAttack & ca)
{
	battleAI->battleCatapultAttacked(ca);
}

void CAdventureAI::battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side)
{
	assert(!battleAI);
	assert(cbc);
	battleAI = CDynLibHandler::getNewBattleAI(battleAIName);
	battleAI->init(cbc);
	battleAI->battleStart(army1, army2, tile, hero1, hero2, side);
}

void CAdventureAI::battleStacksAttacked(const std::vector<BattleStackAttacked> & bsa)
{
	battleAI->battleStacksAttacked(bsa);
}

void CAdventureAI::actionStarted(const BattleAction *action)
{
	battleAI->actionStarted(action);
}

void CAdventureAI::battleNewRoundFirst(int round)
{
	battleAI->battleNewRoundFirst(round);
}

void CAdventureAI::actionFinished(const BattleAction *action)
{
	battleAI->actionFinished(action);
}

void CAdventureAI::battleStacksEffectsSet(const SetStackEffect & sse)
{
	battleAI->battleStacksEffectsSet(sse);
}

void CAdventureAI::battleStacksRemoved(const BattleStacksRemoved & bsr)
{
	battleAI->battleStacksRemoved(bsr);
}

void CAdventureAI::battleObstaclesRemoved(const std::set<si32> & removedObstacles)
{
	battleAI->battleObstaclesRemoved(removedObstacles);
}

void CAdventureAI::battleNewStackAppeared(const CStack * stack)
{
	battleAI->battleNewStackAppeared(stack);
}

void CAdventureAI::battleStackMoved(const CStack * stack, THex dest, int distance, bool end)
{
	battleAI->battleStackMoved(stack, dest, distance, end);
}

void CAdventureAI::battleAttack(const BattleAttack *ba)
{
	battleAI->battleAttack(ba);
}

void CAdventureAI::battleSpellCast(const BattleSpellCast *sc)
{
	battleAI->battleSpellCast(sc);
}

void CAdventureAI::battleEnd(const BattleResult *br)
{
	battleAI->battleEnd(br);
	delNull(battleAI);
}

void CAdventureAI::battleStacksHealedRes(const std::vector<std::pair<ui32, ui32> > & healedStacks, bool lifeDrain, bool tentHeal, si32 lifeDrainFrom)
{
	battleAI->battleStacksHealedRes(healedStacks, lifeDrain, tentHeal, lifeDrainFrom);
}

BattleAction CAdventureAI::activeStack(const CStack * stack)
{
	return battleAI->activeStack(stack);
}