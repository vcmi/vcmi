#ifndef __CGAMESTATE_H__
#define __CGAMESTATE_H__
#include "../global.h"
#include <cassert>

#ifndef _MSC_VER
#include "../hch/CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "map.h"
#endif

#include <set>
#include <vector>
#include <list>
#include "HeroBonus.h"
#include "CCreatureSet.h"

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
class IGameCallback;
class CLuaCallback;
class CCPPObjectScript;
class CCreatureSet;
class CStack;
class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class CGDwelling;
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
struct TerrainTile;
class CHeroClass;
class CCampaign;
class CCampaignState;

namespace boost
{
	class shared_mutex;
}

struct DLL_EXPORT InfoAboutHero
{
private:
	void assign(const InfoAboutHero & iah);
public:
	struct DLL_EXPORT Details
	{
		std::vector<int> primskills;
		int mana, luck, morale;
	} *details;

	char owner;
	const CHeroClass *hclass;
	std::string name;
	int portrait;
	CCreatureSet army; //numbers of creatures are exact numbers if detailed else they are quantity ids (0 - a few, 1 - several and so on)

	InfoAboutHero();
	InfoAboutHero(const InfoAboutHero & iah);
	InfoAboutHero & operator=(const InfoAboutHero & iah);
	~InfoAboutHero();
	void initFromHero(const CGHeroInstance *h, bool detailed);
};

// typedef si32 TResourceUnit;
// typedef std::vector<si32> TResourceVector;
// typedef std::set<si32> TResourceSet;

struct DLL_EXPORT SThievesGuildInfo
{
	std::vector<ui8> playerColors; //colors of players that are in-game

	std::vector< std::list< ui8 > > numOfTowns, numOfHeroes, gold, woodOre, mercSulfCrystGems, obelisks, artifacts, army, income; // [place] -> [colours of players]

	std::map<ui8, InfoAboutHero> colorToBestHero; //maps player's color to his best heros' 

	std::map<ui8, si8> personality; // color to personality // -1 - human, AI -> (00 - random, 01 -  warrior, 02 - builder, 03 - explorer)
	std::map<ui8, si32> bestCreature; // color to ID // id or -1 if not known

// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & playerColors & numOfTowns & numOfHeroes & gold & woodOre & mercSulfCrystGems & obelisks & artifacts & army & income;
// 		h & colorToBestHero & personality & bestCreature;
// 	}

};

struct DLL_EXPORT PlayerState : public CBonusSystemNode
{
public:
	enum EStatus {INGAME, LOSER, WINNER};
	ui8 color;
	ui8 human; //true if human controlled player, false for AI
	ui32 currentSelection; //id of hero/town, 0xffffffff if none
	ui8 team;
	//std::vector<std::vector<std::vector<ui8> > > * fogOfWarMap; //pointer to team's fog of war
	std::vector<si32> resources;
	std::vector<CGHeroInstance *> heroes;
	std::vector<CGTownInstance *> towns;
	std::vector<CGHeroInstance *> availableHeroes; //heroes available in taverns
	std::vector<CGDwelling *> dwellings; //used for town growth

	ui8 enteredWinningCheatCode, enteredLosingCheatCode; //if true, this player has entered cheat codes for loss / victory
	ui8 status; //0 - in game, 1 - loser, 2 - winner <- uses EStatus enum
	ui8 daysWithoutCastle;

	PlayerState();

	//override
	void getParents(TCNodes &out, const CBonusSystemNode *root = NULL) const; 
	void getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root = NULL) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & color & human & currentSelection & team & resources & status;
		h & heroes & towns & availableHeroes & dwellings & bonuses & status & daysWithoutCastle;
		h & enteredLosingCheatCode & enteredWinningCheatCode;
		h & static_cast<CBonusSystemNode&>(*this);
	}
};

struct DLL_EXPORT TeamState : public CBonusSystemNode
{
public:
	ui8 id; //position in gameState::teams
	std::set<ui8> players; // members of this team
	std::vector<std::vector<std::vector<ui8> > >  fogOfWarMap; //true - visible, false - hidden
	
	//TeamState();
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & players & fogOfWarMap;
		h & static_cast<CBonusSystemNode&>(*this);
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

//only for use in BattleInfo
struct DLL_EXPORT SiegeInfo
{
	ui8 wallState[8]; //[0] - keep, [1] - bottom tower, [2] - bottom wall, [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate; 1 - intact, 2 - damaged, 3 - destroyed

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & wallState;
	}
};

struct DLL_EXPORT BattleInfo : public CBonusSystemNode
{
	ui8 side1, side2; //side1 - attacker, side2 - defender
	si32 round, activeStack;
	ui8 siege; //    = 0 ordinary battle    = 1 a siege with a Fort    = 2 a siege with a Citadel    = 3 a siege with a Castle
	si32 tid; //used during town siege - id of attacked town; -1 if not town defence
	int3 tile; //for background and bonuses
	CGHeroInstance *heroes[2];
	CArmedInstance *belligerents[2]; //may be same as heroes
	std::vector<CStack*> stacks;
	std::vector<CObstacleInstance> obstacles;
	ui8 castSpells[2]; //[0] - attacker, [1] - defender
	SiegeInfo si;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side1 & side2 & round & activeStack & siege & tid & tile & stacks & belligerents & obstacles
			& castSpells & si;
		h & heroes;
		h & static_cast<CBonusSystemNode&>(*this);
	}

	//////////////////////////////////////////////////////////////////////////
	void getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root = NULL) const;
	//////////////////////////////////////////////////////////////////////////

	const CStack * getNextStack() const; //which stack will have turn after current one
	void getStackQueue(std::vector<const CStack *> &out, int howMany, int turn = 0, int lastMoved = -1) const; //returns stack in order of their movement action
	CStack * getStack(int stackID, bool onlyAlive = true);
	const CStack * getStack(int stackID, bool onlyAlive = true) const;
	CStack * getStackT(int tileID, bool onlyAlive = true);
	const CStack * getStackT(int tileID, bool onlyAlive = true) const;
	void getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<int> & occupyable, bool flying, int stackToOmmit=-1) const; //send pointer to at least 187 allocated bytes
	static bool isAccessible(int hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos); //helper for makeBFS
	void makeBFS(int start, bool*accessibility, int *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying, bool fillPredecessors) const; //*accessibility must be prepared bool[187] array; last two pointers must point to the at least 187-elements int arrays - there is written result
	std::pair< std::vector<int>, int > getPath(int start, int dest, bool*accessibility, bool flyingCreature, bool twoHex, bool attackerOwned); //returned value: pair<path, length>; length may be different than number of elements in path since flying vreatures jump between distant hexes
	std::vector<int> getAccessibility(int stackID, bool addOccupiable) const; //returns vector of accessible tiles (taking into account the creature range)

	bool isStackBlocked(int ID); //returns true if there is neighboring enemy stack
	static signed char mutualPosition(int hex1, int hex2); //returns info about mutual position of given hexes (-1 - they're distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	static std::vector<int> neighbouringTiles(int hex);
	static si8 getDistance(int hex1, int hex2); //returns distance between given hexes
	ui32 calculateDmg(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky); //charge - number of hexes travelled before attack (for champion's jousting)
	std::pair<ui32, ui32> calculateDmgRange(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero, bool shooting, ui8 charge, bool lucky); //charge - number of hexes travelled before attack (for champion's jousting); returns pair <min dmg, max dmg>
	void calculateCasualties(std::map<ui32,si32> *casualties) const; //casualties are array of maps size 2 (attacker, defeneder), maps are (crid => amount)
	std::set<CStack*> getAttackedCreatures(const CSpell * s, int skillLevel, ui8 attackerOwner, int destinationTile); //calculates stack affected by given spell
	static int calculateSpellDuration(const CSpell * spell, const CGHeroInstance * caster, int usedSpellPower);
	CStack * generateNewStack(const CStackInstance &base, int stackID, bool attackerOwned, int slot, int /*TerrainTile::EterrainType*/ terrain, int position) const; //helper for CGameHandler::setupBattle and spells addign new stacks to the battlefield
	ui32 getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //returns cost of given spell
	int hexToWallPart(int hex) const; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	int lineToWallHex(int line) const; //returns hex with wall in given line
	std::pair<const CStack *, int> getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const; //if attackerOwned is indetermnate, returened stack is of any owner; hex is the number of hex we should be looking from; returns (nerarest creature, predecessorHex)
	ui32 calculateSpellBonus(ui32 baseDamage, const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature) const;
	ui32 calculateSpellDmg(const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower) const; //calculates damage inflicted by spell
	ui32 calculateHealedHP(const CGHeroInstance * caster, const CSpell * spell, const CStack * stack) const;
	si8 hasDistancePenalty(int stackID, int destHex); //determines if given stack has distance penalty shooting given pos
	si8 sameSideOfWall(int pos1, int pos2); //determines if given positions are on the same side of wall
	si8 hasWallPenalty(int stackID, int destHex); //determines if given stack has wall penalty shooting given pos
	si8 canTeleportTo(int stackID, int destHex, int telportLevel); //determines if given stack can teleport to given place
};

class DLL_EXPORT CStack : public CStackInstance
{ 
public:
	ui32 ID; //unique ID of stack
	ui32 baseAmount;
	ui32 firstHPleft; //HP of first creature in stack
	ui8 owner, slot;  //owner - player colour (255 for neutrals), slot - position in garrison (may be 255 for neutrals/called creatures)
	ui8 attackerOwned; //if true, this stack is owned by attakcer (this one from left hand side of battle)
	si16 position; //position on battlefield; -2 - keep, -3 - lower tower, -4 - upper tower
	ui8 counterAttacks; //how many counter attacks can be performed more in this turn (by default set at the beginning of the round to 1)
	si16 shots; //how many shots left

	std::set<ECombatInfo> state;
	struct StackEffect
	{
		ui16 id; //spell id
		ui8 level; //skill level
		si16 turnsRemain; 
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id & level & turnsRemain;
		}
	};
	std::vector<StackEffect> effects;

	//overrides
	const CCreature* getCreature() const {return type;}

	CStack(const CStackInstance *base, int O, int I, bool AO, int S); //c-tor
	CStack() : ID(-1), baseAmount(-1), firstHPleft(-1), owner(255), slot(255), attackerOwned(true), position(-1), counterAttacks(1) {} //c-tor
	const StackEffect * getEffect(ui16 id, int turn = 0) const; //effect id (SP)
	ui8 howManyEffectsSet(ui16 id) const; //returns amount of effects with given id set for this stack
	bool willMove(int turn = 0) const; //if stack has remaining move this turn
	bool moved(int turn = 0) const; //if stack was already moved this turn
	bool canMove(int turn = 0) const; //if stack can move
	ui32 Speed(int turn = 0) const; //get speed of creature with all modificators

	bool doubleWide() const;
	int occupiedHex() const; //returns number of occupied hex (not the position) if stack is double wide; otherwise -1

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CStackInstance&>(*this);
		h & ID & baseAmount & firstHPleft & owner & slot & attackerOwned & position & state & counterAttacks
			& shots;
	}
	bool alive() const //determines if stack is alive
	{
		return vstd::contains(state,ALIVE);
	}
};

class DLL_EXPORT CMP_stack
{
	int phase; //rules of which phase will be used
	int turn;
public:

	bool operator ()(const CStack* a, const CStack* b);
	CMP_stack(int Phase = 1, int Turn = 0);
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
	bool accessible; //true if a hero can be on this node
	int dist; //distance from the first node of searching; -1 is infinity
	CPathNode * theNodeBefore;
	int3 coord; //coordiantes
	bool visited;
};

struct CGPathNode
{
	enum 
	{
		ACCESSIBLE=1, //tile can be entered and passed
		VISITABLE, //tile can be entered as the last tile in path
		BLOCKVIS,  //visitable from neighbouring tile but not passable
		BLOCKED, //tile can't be entered nor visited
		FLYABLE //if hero flies, he can pass this tile
	};

	ui8 accessible; //the enum above
	ui8 land;
	ui8 turns;
	ui32 moveRemains;
	CGPathNode * theNodeBefore;
	int3 coord; //coordinates
	CGPathNode();
};


struct DLL_EXPORT CPath
{
	std::vector<CPathNode> nodes; //just get node by node

	int3 startPos() const; // start point
	int3 endPos() const; //destination point
	void convert(ui8 mode); //mode=0 -> from 'manifest' to 'object'
};

struct DLL_EXPORT CGPath
{
	std::vector<CGPathNode> nodes; //just get node by node

	int3 startPos() const; // start point
	int3 endPos() const; //destination point
	void convert(ui8 mode); //mode=0 -> from 'manifest' to 'object'
};

struct DLL_EXPORT CPathsInfo
{
	const CGHeroInstance *hero;
	int3 hpos;
	int3 sizes;
	CGPathNode ***nodes; //[w][h][level]

	bool getPath(const int3 &dst, CGPath &out);
	CPathsInfo(const int3 &Sizes);
	~CPathsInfo();
};

class DLL_EXPORT CGameState
{
public:
	StartInfo* scenarioOps;
	CCampaignState *campaign;
	ui32 seed;
	ui8 currentPlayer; //ID of player currently having turn
	BattleInfo *curB; //current battle
	ui32 day; //total number of days in game
	Mapa * map;
	std::map<ui8, PlayerState> players; //ID <-> player state
	std::map<ui8, TeamState> teams; //ID <-> team state
	std::map<int, CGDefInfo*> villages, forts, capitols; //def-info for town graphics
	CBonusSystemNode globalEffects;

	struct DLL_EXPORT HeroesPool
	{
		std::map<ui32,CGHeroInstance *> heroesPool; //[subID] - heroes available to buy; NULL if not available
		std::map<ui32,ui8> pavailable; // [subid] -> which players can recruit hero (binary flags)

		CGHeroInstance * pickHeroFor(bool native, int player, const CTown *town, std::map<ui32,CGHeroInstance *> &available, const CHeroClass *bannedClass = NULL) const;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & heroesPool & pavailable;
		}
	} hpool; //we have here all heroes available on this map that are not hired

	boost::shared_mutex *mx;
	PlayerState *getPlayer(ui8 color, bool verbose = true);
	TeamState *getTeam(ui8 teamID);//get team by team ID
	TeamState *getPlayerTeam(ui8 color);// get team by player color
	const PlayerState *getPlayer(ui8 color, bool verbose = true) const;
	const TeamState *getTeam(ui8 teamID) const;
	const TeamState *getPlayerTeam(ui8 color) const;
	void init(StartInfo * si, ui32 checksum, int Seed);
	void loadTownDInfos();
	void randomizeObject(CGObjectInstance *cur);
	std::pair<int,int> pickObject(CGObjectInstance *obj); //chooses type of object to be randomized, returns <type, subtype>
	int pickHero(int owner);
	void apply(CPack *pack);
	CGHeroInstance *getHero(int objid);
	CGTownInstance *getTown(int objid);
	const CGHeroInstance *getHero(int objid) const;
	const CGTownInstance *getTown(int objid) const;
	bool battleCanFlee(int player); //returns true if player can flee from the battle
	int battleGetStack(int pos, bool onlyAlive); //returns ID of stack at given tile
	int battleGetBattlefieldType(int3 tile = int3());//   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	const CGHeroInstance * battleGetOwner(int stackID); //returns hero that owns given stack; NULL if none
	si8 battleMaxSpellLevel(); //calculates maximum spell level possible to be cast on battlefield - takes into account artifacts of both heroes; if no effects are set, SPELL_LEVELS is returned
	bool battleCanShoot(int ID, int dest); //determines if stack with given ID shoot at the selected destination
	UpgradeInfo getUpgradeInfo(const CStackInstance &stack);
	int getPlayerRelations(ui8 color1, ui8 color2);// 0 = enemy, 1 = ally, 2 = same player
	//float getMarketEfficiency(int player, int mode=0);
	std::set<int> getBuildingRequiments(const CGTownInstance *t, int ID);
	int canBuildStructure(const CGTownInstance *t, int ID);// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	bool checkForVisitableDir(const int3 & src, const int3 & dst) const; //check if src tile is visitable from dst tile
	bool checkForVisitableDir(const int3 & src, const TerrainTile *pom, const int3 & dst) const; //check if src tile is visitable from dst tile
	bool getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret); //calculates path between src and dest; returns pointer to newly allocated CPath or NULL if path does not exists
	void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out, int3 src = int3(-1,-1,-1), int movement = -1); //calculates possible paths for hero, by default uses current hero position and movement left; returns pointer to newly allocated CPath or NULL if path does not exists
	int3 guardingCreaturePosition (int3 pos) const;
	int victoryCheck(ui8 player) const; //checks if given player is winner; -1 if std victory, 1 if special victory, 0 else
	int lossCheck(ui8 player) const; //checks if given player is loser;  -1 if std loss, 1 if special, 0 else
	ui8 checkForStandardWin() const; //returns color of player that accomplished standard victory conditions or 255 if no winner
	bool checkForStandardLoss(ui8 player) const; //checks if given player lost the game
	void obtainPlayersStats(SThievesGuildInfo & tgi, int level); //fills tgi with info about other players that is available at given level of thieves' guild
	std::map<ui32,CGHeroInstance *> unusedHeroesFromPool(); //heroes pool without heroes that are available in taverns

	bool isVisible(int3 pos, int player);
	bool isVisible(const CGObjectInstance *obj, int player);

	CGameState(); //c-tor
	~CGameState(); //d-tor
	void getNeighbours(const TerrainTile &srct, int3 tile, std::vector<int3> &vec, const boost::logic::tribool &onLand, bool limitCoastSailing);
	int getMovementCost(const CGHeroInstance *h, const int3 &src, const int3 &dest, int remainingMovePoints=-1, bool checkLast=true);
	int getDate(int mode=0) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & scenarioOps & seed & currentPlayer & day & map & players & teams & hpool & globalEffects & campaign;
		h & villages & forts & capitols;
		if(!h.saving)
		{
			loadTownDInfos();
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
 