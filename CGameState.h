#ifndef CGAMESTATE_H
#define CGAMESTATE_H
#include "global.h"
#ifndef _MSC_VER
#include "hch/CCreatureHandler.h"
#include "lib/VCMI_Lib.h"
#endif
#include <set>
#include <vector>
#ifdef _WIN32
#include <tchar.h>
#else
#include "tchar_amigaos4.h"
#endif

class CTown;
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
	ui32 currentSelection; //id of hero/town, 0xffffffff if none
	std::vector<std::vector<std::vector<ui8> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<si32> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	std::vector<CGHeroInstance *> availableHeroes; //heroes available in taverns
	PlayerState():color(-1),currentSelection(0xffffffff){};
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

	bool isStackBlocked(int ID); //returns true if there is neighbouring enemy stack
	static signed char mutualPosition(int hex1, int hex2); //returns info about mutual position of given hexes (-1 - they're distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	static std::vector<int> neighbouringTiles(int hex);
	static int calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting); //TODO: add additional conditions and require necessary data
	void calculateCasualties(std::set<std::pair<ui32,si32> > *casualties);
};

class DLL_EXPORT CStack
{ 
public:
	ui32 ID; //unique ID of stack
	CCreature * creature;
	ui32 amount, baseAmount;
	ui32 firstHPleft; //HP of first creature in stack
	ui8 owner, slot;  //owner - player colour (255 for neutrals), slot - position in garrison (may be 255 for neutrals/called creatures)
	ui8 attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	ui16 position; //position on battlefield
	ui8 counterAttacks; //how many counter attacks can be performed more in this turn (by default set at the beginning of the round to 1)
	si16 shots; //how many shots left

	std::set<EAbilities> abilities;
	std::set<ECombatInfo> state;
	struct StackEffect
	{
		ui16 id; //spell id
		ui8 level; //skill level
		ui16 turnsRemain; 
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id & level & turnsRemain;
		}
	};
	std::vector<StackEffect> effects;

	CStack(CCreature * C, int A, int O, int I, bool AO, int S);
	CStack() : creature(NULL),amount(-1),owner(255), position(-1), ID(-1), attackerOwned(true), firstHPleft(-1), slot(255), baseAmount(-1), counterAttacks(1){};
	StackEffect * getEffect(ui16 id); //effect id (SP)
	ui32 speed();
	template <typename Handler> void save(Handler &h, const int version)
	{
		h & creature->idNumber;
	}
	template <typename Handler> void load(Handler &h, const int version)
	{
		ui32 id;
		h & id;
		creature = &VLC->creh->creatures[id];
		abilities = creature->abilities;
		shots = creature->shots;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & amount & baseAmount & firstHPleft & owner & slot & attackerOwned & position & state & counterAttacks;
		if(h.saving)
			save(h,version);
		else
			load(h,version);
	}
	bool alive()
	{
		return vstd::contains(state,ALIVE);
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
	std::vector<ui32> resVals;

	struct DLL_EXPORT HeroesPool
	{
		std::map<ui32,CGHeroInstance *> heroesPool; //[subID] - heroes available to buy; NULL if not available
		std::map<ui32,ui8> pavailable; // [subid] -> which players can recruit hero

		CGHeroInstance * pickHeroFor(bool native, int player, const CTown *town, int notThatOne=-1);
	} hpool; //we have here all heroes available on this map that are not hired

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
	float getMarketEfficiency(int player, int mode=0);
	std::set<int3> tilesToReveal(int3 pos, int radious, int player); //if player==-1 => adds all tiles in radious
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
