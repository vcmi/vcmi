#ifndef __CCALLBACK_H__
#define __CCALLBACK_H__

#include "global.h"
#ifdef _WIN32
#include "tchar.h"
#else
#include "tchar_amigaos4.h" //XXX this is mingw header are we need this for something? for 'true'
//support of unicode we should use ICU or some boost wraper areound it
//(boost using this lib during compilation i dont know what for exactly)
#endif
#include "lib/CGameState.h"

/*
 * CCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGHeroInstance;
class CGameState;
struct CPath;
class CGObjectInstance;
class CArmedInstance;
class SComponent;
class IChosen;
class CSelectableComponent;
struct BattleAction;
class CGTownInstance;
struct StartInfo;
class CStack;
struct lua_State;
class CClient;
struct TerrainTile;
class CHeroClass;
class IShipyard;
struct CPackForServer;
class CMapHeader;
struct CGPathNode;
struct CGPath;
class CGGarrison;
class CObstacleInstance;
typedef std::vector<const CStack*> TStacks;

/// Struct which holds a short information about a town
struct InfoAboutTown
{
	struct Details
	{
		int hallLevel, goldIncome;
		bool customRes;
		bool garrisonedHero;

	} *details;

	const CArmedInstance * obj;
	char fortLevel; //0 - none
	char owner;
	std::string name;
	CTown *tType;
	bool built;

	ArmyDescriptor army; //numbers of creatures are valid only if details

	InfoAboutTown();
	~InfoAboutTown();
	void initFromTown(const CGTownInstance *t, bool detailed);
	void initFromGarrison(const CGGarrison *garr, bool detailed);
};

class IBattleCallback
{
public:
	///describes why player cannot cast a specific spell

	enum EStackOwnership
	{
		ONLY_MINE, ONLY_ENEMY, MINE_AND_ENEMY
	};

	bool waitTillRealize; //if true, request functions will return after they are realized by server
	//battle
	virtual int battleGetBattlefieldType()=0; //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	virtual int battleGetObstaclesAtTile(THex tile)=0; //returns bitfield
	virtual std::vector<CObstacleInstance> battleGetAllObstacles()=0; //returns all obstacles on the battlefield
	virtual const CStack * battleGetStackByID(int ID, bool onlyAlive = true)=0; //returns stack info by given ID
	virtual const CStack * battleGetStackByPos(THex pos, bool onlyAlive = true)=0; //returns stack info by given pos
	virtual THex battleGetPos(int stack)=0; //returns position (tile ID) of stack
	virtual int battleMakeAction(BattleAction* action)=0;//for casting spells by hero - DO NOT use it for moving active stack
	virtual TStacks battleGetStacks(EStackOwnership whose = MINE_AND_ENEMY, bool onlyAlive = true)=0; //returns stacks on battlefield
	virtual void getStackQueue( std::vector<const CStack *> &out, int howMany )=0; //returns vector of stack in order of their move sequence
	virtual std::vector<THex> battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<THex> * attackable = NULL)=0; //returns numbers of hexes reachable by creature with id ID
	virtual std::vector<int> battleGetDistances(const CStack * stack, THex hex = THex::INVALID, THex * predecessors = NULL)=0; //returns vector of distances to [dest hex number]
	virtual bool battleCanShoot(const CStack * stack, THex dest)=0; //returns true if unit with id ID can shoot to dest
	virtual bool battleCanCastSpell()=0; //returns true, if caller can cast a spell
	virtual SpellCasting::ESpellCastProblem battleCanCastThisSpell(const CSpell * spell)=0; //determines if given spell can be casted (and returns problem description)
	virtual bool battleCanFlee()=0; //returns true if caller can flee from the battle
	virtual const CGTownInstance * battleGetDefendedTown()=0; //returns defended town if current battle is a siege, NULL instead
	virtual ui8 battleGetWallState(int partOfWall)=0; //for determining state of a part of the wall; format: parameter [0] - keep, [1] - bottom tower, [2] - bottom wall, [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate; returned value: 1 - intact, 2 - damaged, 3 - destroyed; 0 - no battle
	virtual int battleGetWallUnderHex(THex hex)=0; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	virtual TDmgRange battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg = NULL)=0; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	virtual ui8 battleGetSiegeLevel()=0; //returns 0 when there is no siege, 1 if fort, 2 is citadel, 3 is castle
	virtual const CGHeroInstance * battleGetFightingHero(ui8 side) const =0; //returns hero corresponding to given side (0 - attacker, 1 - defender)
	virtual si8 battleHasDistancePenalty(const CStack * stack, THex destHex) =0; //checks if given stack has distance penalty
	virtual si8 battleHasWallPenalty(const CStack * stack, THex destHex) =0; //checks if given stack has wall penalty
	virtual si8 battleCanTeleportTo(const CStack * stack, THex destHex, int telportLevel) =0; //checks if teleportation of given stack to given position can take place
	virtual si8 battleGetTacticDist() =0; //returns tactic distance for calling player or 0 if player is not in tactic phase
	virtual ui8 battleGetMySide() =0; //return side of player in battle (attacker/defender)
	virtual bool battleMakeTacticAction(BattleAction * action) =0; // performs tactic phase actions

	//convienience methods using the ones above
	TStacks battleGetAllStacks() //returns all stacks, alive or dead or undead or mechanical :)
	{
		return battleGetStacks(MINE_AND_ENEMY, false);
	}
};

class ICallback : public virtual IBattleCallback
{
public:
	//hero
	virtual bool moveHero(const CGHeroInstance *h, int3 dst) =0; //dst must be free, neighbouring tile (this function can move hero only by one tile)
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses given hero; true - successfuly, false - not successfuly
	
	//town
	virtual void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)=0;
	virtual bool buildBuilding(const CGTownInstance *town, si32 buildingID)=0;
	virtual void recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount, si32 level=-1)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1)=0; //if newID==-1 then best possible upgrade will be made
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;
	
	virtual void trade(const CGObjectInstance *market, int mode, int id1, int id2, int val1, const CGHeroInstance *hero = NULL)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce
	
	virtual void selectionMade(int selection, int asker) =0;
	virtual int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2)=0;//swaps creatures between two posiibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2)=0;//joins first stack tothe second (creatures must be same type)
	virtual int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2, int val)=0;//split creatures from the first stack
	virtual bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual bool assembleArtifacts(const CGHeroInstance * hero, ui16 artifactSlot, bool assemble, ui32 assembleTo)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, int stackPos)=0;
	virtual void endTurn()=0;
	virtual void buyArtifact(const CGHeroInstance *hero, int aid)=0; //used to buy artifacts in towns (including spell book in the guild and war machines in blacksmith)
	virtual void setFormation(const CGHeroInstance * hero, bool tight)=0;
	virtual void setSelection(const CArmedInstance * obj)=0;

	
	virtual void save(const std::string &fname) = 0;
	virtual void sendMessage(const std::string &mess) = 0;
	virtual void buildBoat(const IShipyard *obj) = 0;

//get info
	virtual bool verifyPath(CPath * path, bool blockSea)const =0;
	virtual int getDate(int mode=0)const =0; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual std::vector< std::vector< std::vector<unsigned char> > > & getVisibilityMap()const =0; //returns visibility map (TODO: make it const)
	virtual int getResourceAmount(int type)const =0;
	virtual bool isVisible(int3 pos)const =0;
	virtual int getMyColor()const =0;
	virtual int getHeroSerial(const CGHeroInstance * hero)const =0;
	virtual const StartInfo * getStartInfo()const =0;
	virtual const CMapHeader * getMapHeader()const =0;
	virtual int getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const =0; //when called during battle, takes into account creatures' spell cost reduction
	virtual int estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const =0; //estimates damage of given spell; returns 0 if spell causes no dmg
	virtual void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj)=0; //get thieves' guild info obtainable while visiting given object
	virtual int3 getGrailPos(float &outKnownRatio)=0;
	virtual int getPlayerStatus(int player) const = 0; //-1 if no such player

	//hero
	virtual int howManyHeroes(bool includeGarrisoned = true)const =0;
	virtual const CGHeroInstance * getHeroInfo(int val, int mode=2)const =0; //mode = 0 -> val = serial; mode = 1 -> val = ID
	virtual std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true)const =0;
	virtual bool getHeroInfo(const CGObjectInstance *hero, InfoAboutHero &dest) const = 0;
	virtual bool getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret)=0;
	virtual const CGPathNode *getPathInfo(int3 tile)=0; //uses main, client pathfinder info
	virtual bool getPath2(int3 dest, CGPath &ret)=0; //uses main, client pathfinder info
	virtual void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out, int3 src = int3(-1,-1,-1), int movement = -1) =0;
	virtual void recalculatePaths()=0; //updates main, client pathfinder info (should be called when moving hero is over)
	virtual void dig(const CGObjectInstance *hero)=0; 
	virtual void castSpell(const CGHeroInstance *hero, int spellID, const int3 &pos = int3(-1, -1, -1))=0; //cast adventure map spell
	
	//map
	virtual std::vector < const CGDwelling * > getMyDwellings() const =0; //returns all dwellings that belong to player
	virtual std::vector < const CGObjectInstance * > getMyObjects() const =0; //returns all objects flagged by belonging player
	virtual std::vector < const CGObjectInstance * > getBlockingObjs(int3 pos)const =0;
	virtual std::vector < const CGObjectInstance * > getVisitableObjs(int3 pos)const =0;
	virtual std::vector < const CGObjectInstance * > getFlaggableObjects(int3 pos) const =0;
	virtual std::vector < std::string > getObjDescriptions(int3 pos)const =0; //returns descriptions of objects at pos in order from the lowest to the highest
	
	//town
	virtual int howManyTowns()const =0;
	virtual const CGTownInstance * getTownInfo(int val, bool mode)const =0; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
	virtual std::vector < const CGTownInstance *> getTownsInfo(bool onlyOur=true) const=0;
	virtual std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const =0; //heroes that can be recruited
	virtual std::string getTavernGossip(const CGObjectInstance * townOrTavern) const =0; 
	virtual int canBuildStructure(const CGTownInstance *t, int ID) =0;//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	virtual std::set<int> getBuildingRequiments(const CGTownInstance *t, int ID) =0;
	virtual bool getTownInfo(const CGObjectInstance *town, InfoAboutTown &dest) const = 0;

	virtual UpgradeInfo getUpgradeInfo(const CArmedInstance *obj, int stackPos)const =0;
	virtual const CCreatureSet* getGarrison(const CGObjectInstance *obj)const =0;

	virtual int3 getMapSize() const =0; //returns size of map - z is 1 for one - level map and 2 for two level map
	virtual const TerrainTile * getTileInfo(int3 tile) const = 0;
	virtual int getPlayerRelations(ui8 color1, ui8 color2) const =0;// 0 = enemy, 1 = ally, 2 = same player 
};

class CBattleCallback : public virtual IBattleCallback
{
private:
	CGameState * gs;
	CBattleCallback(CGameState *GS, int Player, CClient *C);


protected:
	template <typename T> void sendRequest(const T*request);
	CClient *cl;
	virtual bool hasAccess(int playerId) const;
	int player;

public:
	//battle
	int battleGetBattlefieldType() OVERRIDE; //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	int battleGetObstaclesAtTile(THex tile) OVERRIDE; //returns bitfield
	std::vector<CObstacleInstance> battleGetAllObstacles() OVERRIDE; //returns all obstacles on the battlefield
	const CStack * battleGetStackByID(int ID, bool onlyAlive = true) OVERRIDE; //returns stack info by given ID
	const CStack * battleGetStackByPos(THex pos, bool onlyAlive = true) OVERRIDE; //returns stack info by given pos
	THex battleGetPos(int stack) OVERRIDE; //returns position (tile ID) of stack
	int battleMakeAction(BattleAction* action) OVERRIDE;//for casting spells by hero - DO NOT use it for moving active stack
	TStacks battleGetStacks(EStackOwnership whose = MINE_AND_ENEMY, bool onlyAlive = true) OVERRIDE; //returns stacks on battlefield
	void getStackQueue( std::vector<const CStack *> &out, int howMany ) OVERRIDE; //returns vector of stack in order of their move sequence
	std::vector<THex> battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<THex> * attackable = NULL) OVERRIDE; //returns numbers of hexes reachable by creature with id ID
	std::vector<int> battleGetDistances(const CStack * stack, THex hex = THex::INVALID, THex * predecessors = NULL) OVERRIDE; //returns vector of distances to [dest hex number]; if predecessors is not null, it must point to BFIELD_SIZE * sizeof(int) of allocated memory
	bool battleCanShoot(const CStack * stack, THex dest) OVERRIDE; //returns true if unit with id ID can shoot to dest
	bool battleCanCastSpell() OVERRIDE; //returns true, if caller can cast a spell
	SpellCasting::ESpellCastProblem battleCanCastThisSpell(const CSpell * spell) OVERRIDE; //determines if given spell can be casted (and returns problem description)
	bool battleCanFlee() OVERRIDE; //returns true if caller can flee from the battle
	const CGTownInstance * battleGetDefendedTown() OVERRIDE; //returns defended town if current battle is a siege, NULL instead
	ui8 battleGetWallState(int partOfWall) OVERRIDE; //for determining state of a part of the wall; format: parameter [0] - keep, [1] - bottom tower, [2] - bottom wall, [3] - below gate, [4] - over gate, [5] - upper wall, [6] - uppert tower, [7] - gate; returned value: 1 - intact, 2 - damaged, 3 - destroyed; 0 - no battle
	int battleGetWallUnderHex(THex hex) OVERRIDE; //returns part of destructible wall / gate / keep under given hex or -1 if not found
	TDmgRange battleEstimateDamage(const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg = NULL) OVERRIDE; //estimates damage dealt by attacker to defender; it may be not precise especially when stack has randomly working bonuses; returns pair <min dmg, max dmg>
	ui8 battleGetSiegeLevel() OVERRIDE; //returns 0 when there is no siege, 1 if fort, 2 is citadel, 3 is castle
	const CGHeroInstance * battleGetFightingHero(ui8 side) const OVERRIDE; //returns hero corresponding ot given side (0 - attacker, 1 - defender)
	si8 battleHasDistancePenalty(const CStack * stack, THex destHex) OVERRIDE; //checks if given stack has distance penalty
	si8 battleHasWallPenalty(const CStack * stack, THex destHex) OVERRIDE; //checks if given stack has wall penalty
	si8 battleCanTeleportTo(const CStack * stack, THex destHex, int telportLevel) OVERRIDE; //checks if teleportation of given stack to given position can take place
	si8 battleGetTacticDist() OVERRIDE; //returns tactic distance for calling player or 0 if player is not in tactic phase
	ui8 battleGetMySide() OVERRIDE; //return side of player in battle (attacker/defender)
	bool battleMakeTacticAction(BattleAction * action) OVERRIDE; // performs tactic phase actions

	friend class CCallback;
	friend class CClient;
};

class CCallback : public ICallback, public CBattleCallback
{
private:
	CCallback(CGameState * GS, int Player, CClient *C);
	bool isVisible(int3 pos, int Player) const;
	bool isVisible(const CGObjectInstance *obj, int Player) const;
protected:
	virtual bool hasAccess(int playerId) const OVERRIDE;

public:
//commands
	bool moveHero(const CGHeroInstance *h, int3 dst); //dst must be free, neighbouring tile (this function can move hero only by one tile)
	bool teleportHero(const CGHeroInstance *who, const CGTownInstance *where);
	void selectionMade(int selection, int asker);
	int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2);
	int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2); //first goes to the second
	int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, int p1, int p2, int val);
	bool dismissHero(const CGHeroInstance * hero);
	bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2);
	bool assembleArtifacts(const CGHeroInstance * hero, ui16 artifactSlot, bool assemble, ui32 assembleTo);
	bool buildBuilding(const CGTownInstance *town, si32 buildingID);
	void recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount, si32 level=-1);
	bool dismissCreature(const CArmedInstance *obj, int stackPos);
	bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1);
	void endTurn();
	void swapGarrisonHero(const CGTownInstance *town);
	void buyArtifact(const CGHeroInstance *hero, int aid);
	void trade(const CGObjectInstance *market, int mode, int id1, int id2, int val1, const CGHeroInstance *hero = NULL);
	void setFormation(const CGHeroInstance * hero, bool tight);
	void setSelection(const CArmedInstance * obj);
	void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero);
	void save(const std::string &fname);
	void sendMessage(const std::string &mess);
	void buildBoat(const IShipyard *obj);
	void dig(const CGObjectInstance *hero); 
	void castSpell(const CGHeroInstance *hero, int spellID, const int3 &pos = int3(-1, -1, -1));

//get info
	bool verifyPath(CPath * path, bool blockSea) const;
	int getDate(int mode=0) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	std::vector< std::vector< std::vector<unsigned char> > > & getVisibilityMap() const; //returns visibility map (TODO: make it const)
	const CGHeroInstance * getHeroInfo(int val, int mode=2) const; //mode = 0 -> val = serial; mode = 1 -> val = hero type id (subID); mode = 2 -> val = global object serial id (id)
	const CGObjectInstance * getObjectInfo(int ID) const; //global object serial id (ID)
	int getResourceAmount(int type) const;
	std::vector<si32> getResourceAmount() const;
	int howManyHeroes(bool includeGarrisoned = true) const;
	const CGTownInstance * getTownInfo(int val, bool mode) const; //mode = 0 -> val = player town serial; mode = 1 -> val = object id (serial)
	std::vector < const CGTownInstance *> getTownsInfo(bool onlyOur=true) const;
	int howManyTowns()const;
	std::vector < std::string > getObjDescriptions(int3 pos) const; //returns descriptions of objects at pos in order from the lowest to the highest
	std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true) const;
	bool isVisible(int3 pos) const;
	int getMyColor() const;
	int getHeroSerial(const CGHeroInstance * hero) const;
	//int getMySerial() const;
	const CCreatureSet* getGarrison(const CGObjectInstance *obj) const;
	UpgradeInfo getUpgradeInfo(const CArmedInstance *obj, int stackPos) const;
	const StartInfo * getStartInfo() const;
	const CMapHeader * getMapHeader()const ;
	int getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const; //when called during battle, takes into account creatures' spell cost reduction
	int estimateSpellDamage(const CSpell * sp, const CGHeroInstance * hero) const; //estimates damage of given spell; returns 0 if spell causes no dmg
	void getThievesGuildInfo(SThievesGuildInfo & thi, const CGObjectInstance * obj); //get thieves' guild info obtainable while visiting given object
	int3 getGrailPos(float &outKnownRatio); //returns pos and (via arg) percent of discovered obelisks; TODO: relies on fairness of GUI/AI... :/

	std::vector < const CGDwelling * > getMyDwellings() const; //returns all dwellings that belong to player
	std::vector < const CGObjectInstance * > getMyObjects() const; //returns all objects flagged by belonging player
	std::vector < const CGObjectInstance * > getBlockingObjs(int3 pos) const;
	std::vector < const CGObjectInstance * > getVisitableObjs(int3 pos) const;
	std::vector < const CGObjectInstance * > getFlaggableObjects(int3 pos) const;
	int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map
	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGObjectInstance * townOrTavern) const; //heroes that can be recruited
	std::string getTavernGossip(const CGObjectInstance * townOrTavern) const;
	const TerrainTile * getTileInfo(int3 tile) const;
	int canBuildStructure(const CGTownInstance *t, int ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements
	std::set<int> getBuildingRequiments(const CGTownInstance *t, int ID);
	bool getPath(int3 src, int3 dest, const CGHeroInstance * hero, CPath &ret);
	const CGPathNode *getPathInfo(int3 tile);
	bool getPath2(int3 dest, CGPath &ret);
	void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out, int3 src = int3(-1,-1,-1), int movement = -1); //calculates possible paths for hero, by default uses current hero position and movement left;
	void recalculatePaths(); //updates pathfinder info (should be called when moving hero is over)
	bool getHeroInfo(const CGObjectInstance *hero, InfoAboutHero &dest) const;
	bool getTownInfo(const CGObjectInstance *town, InfoAboutTown &dest) const;
	int3 guardingCreaturePosition (int3 pos) const;
	int getPlayerStatus(int player) const;
	int getPlayerRelations(ui8 color1, ui8 color2) const;// 0 = enemy, 1 = ally, 2 = same player 


//XXX hmmm _tmain on _GNUC_ wtf?
//friends
	friend class CClient;
#ifndef __GNUC__
	friend int _tmain(int argc, _TCHAR* argv[]);
#else
	friend int main(int argc, char** argv);
#endif
};

#endif // __CCALLBACK_H__
