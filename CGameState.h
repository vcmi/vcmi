#ifndef CGAMESTATE_H
#define CGAMESTATE_H
#include "global.h"
#ifndef _MSC_VER
#include "../hch/CCreatureHandler.h"
#include "lib/VCMI_Lib.h"
#endif
#include <set>
#include <vector>
#ifdef _WIN32
#include <tchar.h>
#else
#include "tchar_amigaos4.h"
#endif

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

namespace boost
{
	class shared_mutex;
}

struct DLL_EXPORT PlayerState
{
public:
	ui8 color, serial;
	std::vector<std::vector<std::vector<ui8> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<si32> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	PlayerState():color(-1){};
};

struct DLL_EXPORT BattleInfo
{
	ui8 side1, side2;
	si32 round, activeStack;
	ui8 siege; //    = 0 ordinary battle    = 1 a siege with a Fort    = 2 a siege with a Citadel    = 3 a siege with a Castle
	int3 tile; //for background and bonuses
	si32 hero1, hero2;
	CCreatureSet army1, army2;
	std::vector<CStack*> stacks;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side1 & side2 & round & activeStack & siege & tile & stacks & army1 & army2 & hero1 & hero2;
	}
	CStack * getStack(int stackID);
	CStack * getStackT(int tileID);
	void getAccessibilityMap(bool *accessibility, int stackToOmmit=-1); //send pointer to at least 187 allocated bytes
	void getAccessibilityMapForTwoHex(bool *accessibility, bool atackerSide, int stackToOmmit=-1); //send pointer to at least 187 allocated bytes
	void makeBFS(int start, bool*accessibility, int *predecessor, int *dists); //*accessibility must be prepared bool[187] array; last two pointers must point to the at least 187-elements int arrays - there is written result
	std::vector<int> getPath(int start, int dest, bool*accessibility);
	std::vector<int> getAccessibility(int stackID); //returns vector of accessible tiles (taking into account the creature range)

	static signed char mutualPosition(int hex1, int hex2); //returns info about mutual position of given hexes (-1 - they're distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	static std::vector<int> neighbouringTiles(int hex);
	static int calculateDmg(const CStack* attacker, const CStack* defender); //TODO: add additional conditions and require necessary data
};

class DLL_EXPORT CStack
{
public:
	ui32 ID; //unique ID of stack
	CCreature * creature;
	ui32 amount;
	ui32 firstHPleft; //HP of first creature in stack
	ui8 owner;
	ui8 attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	ui16 position; //position on battlefield
	ui8 alive; //true if it is alive

	CStack(CCreature * C, int A, int O, int I, bool AO);
	CStack() : creature(NULL),amount(-1),owner(255), alive(true), position(-1), ID(-1), attackerOwned(true), firstHPleft(-1){};

	template <typename Handler> void save(Handler &h, const int version)
	{
		h & creature->idNumber;
	}
	template <typename Handler> void load(Handler &h, const int version)
	{
		ui32 id;
		h & id;
		creature = &VLC->creh->creatures[id];
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & amount & firstHPleft & owner & attackerOwned & position & alive;
		if(h.saving)
			save(h,version);
		else
			load(h,version);
	}
};

struct UpgradeInfo
{
	int oldID; //creature to be upgraded
	std::vector<int> newID; //possible upgrades
	std::vector<std::set<std::pair<int,int> > > cost; // cost[upgrade_serial] -> set of pairs<resource_ID,resource_amount>
	UpgradeInfo(){oldID = -1;};
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
	std::map<int, CGDefInfo*> villages, forts, capitols; //def-info for town graphics

	boost::shared_mutex *mx;

	CGameState();
	~CGameState();
	void init(StartInfo * si, Mapa * map, int Seed);
	void applyNL(IPack * pack);
	void apply(IPack * pack);
	void randomizeObject(CGObjectInstance *cur);
	std::pair<int,int> pickObject(CGObjectInstance *obj);
	int pickHero(int owner);

	CGHeroInstance *getHero(int objid);
	CGTownInstance *getTown(int objid);

	bool battleMoveCreatureStack(int ID, int dest);
	bool battleAttackCreatureStack(int ID, int dest);
	bool battleShootCreatureStack(int ID, int dest);
	int battleGetStack(int pos); //returns ID of stack at given tile
	UpgradeInfo getUpgradeInfo(CArmedInstance *obj, int stackPos);
public:
	int getDate(int mode=0) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	
	friend class CCallback;
	friend class CPathfinder;;
	friend class CLuaCallback;
	friend class CClient;
	friend void initGameState(Mapa * map, CGameInfo * cgi);
	friend class CScriptCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};

#endif //CGAMESTATE_H
