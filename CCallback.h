#ifndef CCALLBACK_H
#define CCALLBACK_H

#include "mapHandler.h"
#ifdef _WIN32
#include "tchar.h"
#else
#include "tchar_amigaos4.h"
#endif
#include "CGameState.h"

class CGameState;
struct CPath;
class CGObjectInstance;
class SComponent;
class IChosen;
class CSelectableComponent;
struct BattleAction;
typedef struct lua_State lua_State;

//structure gathering info about upgrade possibilites
struct UpgradeInfo
{
	int oldID; //creature to be upgraded
	std::vector<int> newID; //possible upgrades
	std::vector<std::set<std::pair<int,int> > > cost; // cost[upgrade_serial] -> set of pairs<resource_ID,resource_amount>
	UpgradeInfo(){oldID = -1;};
};

class ICallback
{
public:
	virtual bool moveHero(int ID, CPath * path, int idtype, int pathType=0)=0;//idtype: 0 - position in vector of heroes (of that player); 1 - ID of hero
															//pathType: 0 - nodes are manifestation pos, 1 - nodes are object pos
	virtual int swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)=0;//swaps creatures between two posiibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)=0;//joins first stack tothe second (creatures must be same type)
	virtual int splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val)=0;//split creatures from the first stack
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses diven hero; true - successfuly, false - not successfuly
	virtual bool swapArifacts(const CGHeroInstance * hero1, bool worn1, int pos1, const CGHeroInstance * hero2, bool worn2, int pos2)=0; //swaps artifacts between two given heroes
	virtual void recruitCreatures(const CGObjectInstance *obj, int ID, int amount)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, int stackPos)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1)=0; //if newID==-1 then best possible upgrade will be made

//get info
	virtual bool verifyPath(CPath * path, bool blockSea)=0;
	virtual int getDate(int mode=0)=0; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual PseudoV< PseudoV< PseudoV<unsigned char> > > & getVisibilityMap()=0; //returns visibility map (TODO: make it const)
	virtual const CGHeroInstance * getHeroInfo(int player, int val, bool mode)=0; //mode = 0 -> val = serial; mode = 1 -> val = ID
	virtual int getResourceAmount(int type)=0;
	virtual int howManyHeroes()=0;
	virtual const CGTownInstance * getTownInfo(int val, bool mode)=0; //mode = 0 -> val = serial; mode = 1 -> val = ID
	virtual int howManyTowns()=0;
	virtual std::vector < std::string > getObjDescriptions(int3 pos)=0; //returns descriptions of objects at pos in order from the lowest to the highest
	virtual std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true)=0;
	virtual bool isVisible(int3 pos)=0;
	virtual int getMyColor()=0;
	virtual int getMySerial()=0;
	virtual int getHeroSerial(const CGHeroInstance * hero)=0;
	virtual const CCreatureSet* getGarrison(const CGObjectInstance *obj)=0;
	virtual UpgradeInfo getUpgradeInfo(const CArmedInstance *obj, int stackPos)=0;

//battle
	virtual int battleGetBattlefieldType()=0; //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	virtual int battleGetObstaclesAtTile(int tile)=0; //returns bitfield
	virtual int battleGetStack(int pos)=0; //returns ID of stack on the tile
	virtual CStack battleGetStackByID(int ID)=0; //returns stack info by given ID
	virtual CStack battleGetStackByPos(int pos)=0; //returns stack info by given pos
	virtual int battleGetPos(int stack)=0; //returns position (tile ID) of stack
	//virtual int battleMakeAction(BattleAction* action)=0;//perform action with an active stack (or custom action)
	virtual std::map<int, CStack> battleGetStacks()=0; //returns stacks on battlefield
	virtual CCreature battleGetCreature(int number)=0; //returns type of creature by given number of stack
	//virtual bool battleMoveCreature(int ID, int dest)=0; //moves creature with id ID to dest if possible
	virtual std::vector<int> battleGetAvailableHexes(int ID)=0; //reutrns numbers of hexes reachable by creature with id ID
	virtual bool battleIsStackMine(int ID)=0; //returns true if stack with id ID belongs to caller
	virtual bool battleCanShoot(int ID, int dest)=0; //returns true if unit with id ID can shoot to dest
};

struct HeroMoveDetails
{
	int3 src, dst; //source and destination points
	CGHeroInstance * ho; //object instance of this hero
	int owner;
	bool successful;
};

class CCallback : public ICallback
{
private:
	void newTurn();
	CCallback(CGameState * GS, int Player):gs(GS),player(Player){};
	CGameState * gs;
	int lowestSpeed(CGHeroInstance * chi); //speed of the slowest stack
	int valMovePoints(CGHeroInstance * chi);
	bool isVisible(int3 pos, int Player);

protected:
	int player;

public:
//commands
	bool moveHero(int ID, CPath * path, int idtype, int pathType=0);//idtype: 0 - position in vector of heroes (of that player); 1 - ID of hero
															//pathType: 0 - nodes are manifestation pos, 1 - nodes are object pos
	void selectionMade(int selection, int asker);
	int swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2);
	int mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2); //first goes to the second
	int splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val);
	bool dismissHero(const CGHeroInstance * hero);
	bool swapArifacts(const CGHeroInstance * hero1, bool worn1, int pos1, const CGHeroInstance * hero2, bool worn2, int pos2);
	bool buildBuilding(const CGTownInstance *town, int buildingID);
	void recruitCreatures(const CGObjectInstance *obj, int ID, int amount);
	bool dismissCreature(const CArmedInstance *obj, int stackPos);
	bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1);


//get info
	bool verifyPath(CPath * path, bool blockSea);
	int getDate(int mode=0); //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	PseudoV< PseudoV< PseudoV<unsigned char> > > & getVisibilityMap(); //returns visibility map (TODO: make it const)
	const CGHeroInstance * getHeroInfo(int player, int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type);
	std::vector<int> getResourceAmount();
	int howManyHeroes();
	const CGTownInstance * getTownInfo(int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	std::vector < const CGTownInstance *> getTownsInfo(bool onlyOur=true);
	int howManyTowns();
	std::vector < std::string > getObjDescriptions(int3 pos); //returns descriptions of objects at pos in order from the lowest to the highest
	std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true);
	bool isVisible(int3 pos);
	int getMyColor();
	int getHeroSerial(const CGHeroInstance * hero);
	int getMySerial();
	const CCreatureSet* getGarrison(const CGObjectInstance *obj);
	UpgradeInfo getUpgradeInfo(const CArmedInstance *obj, int stackPos);

	//battle
	int battleGetBattlefieldType(); //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	int battleGetObstaclesAtTile(int tile); //returns bitfield
	int battleGetStack(int pos); //returns ID of stack on the tile
	CStack battleGetStackByID(int ID); //returns stack info by given ID
	CStack battleGetStackByPos(int pos); //returns stack info by given pos
	int battleGetPos(int stack); //returns position (tile ID) of stack
	//int battleMakeAction(BattleAction* action);//perform action with an active stack (or custom action)
	std::map<int, CStack> battleGetStacks(); //returns stacks on battlefield
	CCreature battleGetCreature(int number); //returns type of creature by given number of stack
	//bool battleMoveCreature(int ID, int dest); //moves creature with id ID to dest if possible
	std::vector<int> battleGetAvailableHexes(int ID); //reutrns numbers of hexes reachable by creature with id ID
	bool battleIsStackMine(int ID); //returns true if stack with id ID belongs to caller
	bool battleCanShoot(int ID, int dest); //returns true if unit with id ID can shoot to dest



//friends
#ifndef __GNUC__
	friend int _tmain(int argc, _TCHAR* argv[]);
#else
	friend int main(int argc, _TCHAR* argv[]);
#endif
};
class CScriptCallback
{
public:
	CGameState * gs;

	//get info
	static int3 getPos(CGObjectInstance * ob);
	int getHeroOwner(int heroID);
	int getSelectedHero();
	int getDate(int mode=0);

	//do sth
	static void changePrimSkill(int ID, int which, int val);
	void showInfoDialog(int player, std::string text, std::vector<SComponent*> * components); //TODO: obslugiwac nulle
	void showSelDialog(int player, std::string text, std::vector<CSelectableComponent*>*components, IChosen * asker);
	void giveResource(int player, int which, int val);
	void showCompInfo(int player, SComponent * comp);
	void heroVisitCastle(CGObjectInstance * ob, int heroID);
	void stopHeroVisitCastle(CGObjectInstance * ob, int heroID);
	void giveHeroArtifact(int artid, int hid, int position); //pos==-1 - first free slot in backpack
	void startBattle(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2); //use hero=NULL for no hero
	void startBattle(int heroID, CCreatureSet * army, int3 tile); //for hero<=>neutral army

	//friends
	friend void initGameState(CGameInfo * cgi);
};
class CLuaCallback : public CScriptCallback
{
private:

	static void registerFuncs(lua_State * L);
	static int getPos(lua_State * L);//(CGObjectInstance * object);
	static int changePrimSkill(lua_State * L);//(int ID, int which, int val);
	static int getGnrlText(lua_State * L);//(int ID, int which, int val);
	static int getSelectedHero(lua_State * L);//()

	friend void initGameState(CGameInfo * cgi);
};
#endif //CCALLBACK_H
