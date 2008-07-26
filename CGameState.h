#ifndef CGAMESTATE_H
#define CGAMESTATE_H
#include "global.h"
#include <set>
#include <vector>
#include <tchar.h>

class CScriptCallback;
class CCallback;
class CLuaCallback;
class CCPPObjectScript;
class CCreatureSet;
class CStack;
class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class CGDefInfo;
class CObjectScript;
class CGObjectInstance;
class CCreature;
struct Mapa;
struct StartInfo;
struct SDL_Surface;
class CMapHandler;
class CPathfinder;
struct IPack;

struct DLL_EXPORT PlayerState
{
public:
	int color, serial;
	std::vector<std::vector<std::vector<unsigned char> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<int> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	PlayerState():color(-1){};
};

struct DLL_EXPORT BattleInfo
{
	int side1, side2;
	int round, activeStack;
	int siege; //    = 0 ordinary battle    = 1 a siege with a Fort    = 2 a siege with a Citadel    = 3 a siege with a Castle
	int3 tile; //for background and bonuses
	CGHeroInstance *hero1, *hero2;
	CCreatureSet * army1, * army2;
	std::vector<CStack*> stacks;
	bool stackActionPerformed; //true if current stack has been moved
};

class DLL_EXPORT CStack
{
public:
	int ID; //unique ID of stack
	CCreature * creature;
	int amount;
	int firstHPleft; //HP of first creature in stack
	int owner;
	bool attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	int position; //position on battlefield
	bool alive; //true if it is alive
	CStack(CCreature * C, int A, int O, int I, bool AO);
	CStack() : creature(NULL),amount(-1),owner(255), alive(true), position(-1), ID(-1), attackerOwned(true), firstHPleft(-1){};
};

class DLL_EXPORT CGameState
{
private:
	StartInfo* scenarioOps;
	ui32 seed;
	ui8 currentPlayer; //ID of player currently having turn
	BattleInfo *curB; //current battle
	ui32 day; //total number of days in game
	Mapa * map;
	std::map<ui8,PlayerState> players; //ID <-> playerstate
	std::set<CCPPObjectScript *> cppscripts; //C++ scripts
	std::map<int, std::map<std::string, CObjectScript*> > objscr; //non-C++ scripts 
	
	std::map<int, CGDefInfo*> villages, forts, capitols; //def-info for town graphics

	bool checkFunc(int obid, std::string name)
	{
		if (objscr.find(obid)!=objscr.end())
		{
			if(objscr[obid].find(name)!=objscr[obid].end())
			{
				return true;
			}
		}
		return false;
	}

	void init(StartInfo * si, Mapa * map, int Seed);
	void apply(IPack * pack);
	void randomizeObject(CGObjectInstance *cur);
	std::pair<int,int> pickObject(CGObjectInstance *obj);
	int pickHero(int owner);

	void battle(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CArmedInstance *hero1, CArmedInstance *hero2);
	bool battleMoveCreatureStack(int ID, int dest);
	bool battleAttackCreatureStack(int ID, int dest);
	std::vector<int> battleGetRange(int ID); //called by std::vector<int> CCallback::battleGetAvailableHexes(int ID);
public:
	int getDate(int mode=0) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	
	friend class CCallback;
	friend class CPathfinder;;
	friend class CLuaCallback;
	friend class CClient;
	friend void initGameState(Mapa * map, CGameInfo * cgi);
	friend class CScriptCallback;
	friend void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script);
	friend class CMapHandler;
	friend class CGameHandler;
};

#endif //CGAMESTATE_H
