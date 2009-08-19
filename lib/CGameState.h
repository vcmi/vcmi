#ifndef __CGAMESTATE_H__
#define __CGAMESTATE_H__
#include "../global.h"
#ifndef _MSC_VER
#include "../hch/CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "map.h"
#endif
#include <set>
#include <vector>
#include "StackFeature.h"
#ifdef _WIN32
#include <tchar.h>
#else
#include "../tchar_amigaos4.h"
#endif

/*
 * CGameState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
struct SetObjectProperty;
struct MetaString;
struct CPack;
class CSpell;


namespace boost
{
	class shared_mutex;
}

struct DLL_EXPORT PlayerState
{
public:
	ui8 color, serial;
	ui8 human; //true if human controlled player, false for AI
	ui32 currentSelection; //id of hero/town, 0xffffffff if none
	std::vector<std::vector<std::vector<ui8> > >  fogOfWarMap; //true - visible, false - hidden
	std::vector<si32> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	std::vector<CGHeroInstance *> availableHeroes; //heroes available in taverns
	PlayerState():color(-1),currentSelection(0xffffffff){};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & color & serial & human & currentSelection & fogOfWarMap & resources;

		ui32 size;
		if(h.saving) //write subids of available heroes
		{
			size = availableHeroes.size();
			h & size;
			for(size_t i=0; i < size; i++)
				h & availableHeroes[i]->subID;
		}
		else
		{
			ui32 hid; 
			h & size;
			for(size_t i=0; i < size; i++)
			{
				//fill availableHeroes with dummy hero instances, holding subids
				h & hid;
				availableHeroes.push_back(new CGHeroInstance);
				availableHeroes[availableHeroes.size()-1]->subID = hid;
			}
		}
	}
};

struct DLL_EXPORT CObstacleInstance
{
	int uniqueID;
	int ID; //ID of obstacle (defines type of it)
	int pos; //position on battlefield
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & pos & uniqueID;
	}
};

struct DLL_EXPORT BattleInfo
{
	ui8 side1, side2; //side1 - attacker, side2 - defender
	si32 round, activeStack;
	ui8 siege; //    = 0 ordinary battle    = 1 a siege with a Fort    = 2 a siege with a Citadel    = 3 a siege with a Castle
	int3 tile; //for background and bonuses
	si32 hero1, hero2;
	CCreatureSet army1, army2;
	std::vector<CStack*> stacks;
	std::vector<CObstacleInstance> obstacles;
	ui8 castSpells[2]; //[0] - attacker, [1] - defender

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side1 & side2 & round & activeStack & siege & tile & stacks & army1 & army2 & hero1 & hero2 & obstacles
			& castSpells;
	}
	CStack * getNextStack(); //which stack will have turn after current one
	std::vector<CStack> getStackQueue(); //returns stack in order of their movement action
	CStack * getStack(int stackID, bool onlyAlive = true);
	CStack * getStackT(int tileID, bool onlyAlive = true);
	void getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<int> & occupyable, bool flying, int stackToOmmit=-1); //send pointer to at least 187 allocated bytes
	static bool isAccessible(int hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos); //helper for makeBFS
	void makeBFS(int start, bool*accessibility, int *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying); //*accessibility must be prepared bool[187] array; last two pointers must point to the at least 187-elements int arrays - there is written result
	std::pair< std::vector<int>, int > getPath(int start, int dest, bool*accessibility, bool flyingCreature, bool twoHex, bool attackerOwned); //returned value: pair<path, length>; length may be different than number of elements in path since flying vreatures jump between distant hexes
	std::vector<int> getAccessibility(int stackID, bool addOccupiable); //returns vector of accessible tiles (taking into account the creature range)

	bool isStackBlocked(int ID); //returns true if there is neighbouring enemy stack
	static signed char mutualPosition(int hex1, int hex2); //returns info about mutual position of given hexes (-1 - they're distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	static std::vector<int> neighbouringTiles(int hex);
	static int calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting); //TODO: add additional conditions and require necessary data
	void calculateCasualties(std::set<std::pair<ui32,si32> > *casualties);
	std::set<CStack*> getAttackedCreatures(const CSpell * s, const CGHeroInstance * caster, int destinationTile); //calculates stack affected by given spell
	static int calculateSpellDuration(const CSpell * spell, const CGHeroInstance * caster);
	static CStack * generateNewStack(const CGHeroInstance * owner, int creatureID, int amount, int stackID, bool attackerOwned, int slot, int /*TerrainTile::EterrainType*/ terrain, int position); //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
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
	si8 morale, luck; //base stack luck/morale

	std::vector<StackFeature> features;
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

	int valOfFeatures(StackFeature::ECombatFeatures type, int subtype = -1024) const;//subtype -> subtype of bonus, if -1024 then any
	bool hasFeatureOfType(StackFeature::ECombatFeatures type, int subtype = -1024) const; //determines if stack has a bonus of given type (and optionally subtype)

	CStack(CCreature * C, int A, int O, int I, bool AO, int S); //c-tor
	CStack() : ID(-1), creature(NULL), amount(-1), baseAmount(-1), firstHPleft(-1), owner(255), slot(255), attackerOwned(true), position(-1), counterAttacks(1) {} //c-tor
	const StackEffect * getEffect(ui16 id) const; //effect id (SP)
	ui8 howManyEffectsSet(ui16 id) const; //returns amount of effects with given id set for this stack
	bool willMove(); //if stack has remaining move this turn
	ui32 Speed() const; //get speed of creature with all modificators
	si8 Morale() const; //get morale of stack with all modificators
	si8 Luck() const; //get luck of stack with all modificators
	si32 Attack() const; //get attack of stack with all modificators
	si32 Defense(bool withFrenzy = true) const; //get defense of stack with all modificators
	ui16 MaxHealth() const; //get max HP of stack with all modifiers
	template <typename Handler> void save(Handler &h, const int version)
	{
		h & creature->idNumber;
	}
	template <typename Handler> void load(Handler &h, const int version)
	{
		ui32 id;
		h & id;
		creature = &VLC->creh->creatures[id];
		//features = creature->abilities;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & amount & baseAmount & firstHPleft & owner & slot & attackerOwned & position & state & counterAttacks
			& shots & morale & luck & features;
		if(h.saving)
			save(h,version);
		else
			load(h,version);
	}
	bool alive() const //determines if stack is alive
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

struct CPathNode
{
	bool accesible; //true if a hero can be on this node
	int dist; //distance from the first node of searching; -1 is infinity
	CPathNode * theNodeBefore;
	int3 coord; //coordiantes
	bool visited;
};

struct DLL_EXPORT CPath
{
	std::vector<CPathNode> nodes; //just get node by node

	int3 startPos() const; // start point
	int3 endPos() const; //destination point
	void convert(ui8 mode); //mode=0 -> from 'manifest' to 'object'
};

class DLL_EXPORT CGameState
{
public:
	StartInfo* scenarioOps;
	ui32 seed;
	ui8 currentPlayer; //ID of player currently having turn
	BattleInfo *curB; //current battle
	ui32 day; //total number of days in game
	Mapa * map;
	std::map<ui8,PlayerState> players; //ID <-> player state
	std::map<int, CGDefInfo*> villages, forts, capitols; //def-info for town graphics
	std::vector<ui32> resVals; //default values of resources in gold

	struct DLL_EXPORT HeroesPool
	{
		std::map<ui32,CGHeroInstance *> heroesPool; //[subID] - heroes available to buy; NULL if not available
		std::map<ui32,ui8> pavailable; // [subid] -> which players can recruit hero

		CGHeroInstance * pickHeroFor(bool native, int player, const CTown *town, std::map<ui32,CGHeroInstance *> &available) const;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & heroesPool & pavailable;
		}
	} hpool; //we have here all heroes available on this map that are not hired

	boost::shared_mutex *mx;

	PlayerState *getPlayer(ui8 color);
	void init(StartInfo * si, Mapa * map, int Seed);
	void loadTownDInfos();
	void randomizeObject(CGObjectInstance *cur);
	std::pair<int,int> pickObject(CGObjectInstance *obj);
	int pickHero(int owner);
	void apply(CPack *pack);
	CGHeroInstance *getHero(int objid);
	CGTownInstance *getTown(int objid);
	bool battleMoveCreatureStack(int ID, int dest);
	bool battleAttackCreatureStack(int ID, int dest);
	bool battleShootCreatureStack(int ID, int dest);
	bool battleCanFlee(int player); //returns true if player can flee from the battle
	int battleGetStack(int pos, bool onlyAlive); //returns ID of stack at given tile
	int battleGetBattlefieldType(int3 tile = int3());//   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	si8 battleMaxSpellLevel(); //calculates maximum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, SPELL_LEVELS is returned
	UpgradeInfo getUpgradeInfo(CArmedInstance *obj, int stackPos);
	float getMarketEfficiency(int player, int mode=0);
	int canBuildStructure(const CGTownInstance *t, int ID);// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const; //check if dst tile is visitable from dst tile
	bool getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret); //calculates path between src and dest; returns pointer to newly allocated CPath or NULL if path does not exists

	bool isVisible(int3 pos, int player);
	bool isVisible(const CGObjectInstance *obj, int player);

	CGameState(); //c-tor
	~CGameState(); //d-tor
	void getNeighbours(int3 tile, std::vector<int3> &vec, const boost::logic::tribool &onLand);
	int getMovementCost(const CGHeroInstance *h, int3 src, int3 dest, int remainingMovePoints=-1, bool checkLast=true);
	int getDate(int mode=0) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & scenarioOps & seed & currentPlayer & day & map & players & resVals & hpool;
		if(!h.saving)
		{
			loadTownDInfos();

			//recreating towns/heroes vectors in players entries
			for(int i=0; i<map->towns.size(); i++)
				if(map->towns[i]->tempOwner < PLAYER_LIMIT)
					getPlayer(map->towns[i]->tempOwner)->towns.push_back(map->towns[i]);
			for(int i=0; i<map->heroes.size(); i++)
				if(map->heroes[i]->tempOwner < PLAYER_LIMIT)
					getPlayer(map->heroes[i]->tempOwner)->heroes.push_back(map->heroes[i]);
			//recreating available heroes
			for(std::map<ui8,PlayerState>::iterator i=players.begin(); i!=players.end(); i++)
			{
				for(size_t j=0; j < i->second.availableHeroes.size(); j++)
				{
					ui32 hlp = i->second.availableHeroes[j]->subID;
					delete i->second.availableHeroes[j];
					i->second.availableHeroes[j] = hpool.heroesPool[hlp];
				}
			}
		}
	}

	friend class CCallback;
	friend class CLuaCallback;
	friend class CClient;
	friend void initGameState(Mapa * map, CGameInfo * cgi);
	friend class IGameCallback;
	friend class CMapHandler;
	friend class CGameHandler;
};


#endif // __CGAMESTATE_H__
