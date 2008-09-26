#ifndef CCALLBACK_H
#define CCALLBACK_H

#include "global.h"
#ifdef _WIN32
#include "tchar.h"
#else
#include "tchar_amigaos4.h"
#endif
#include "CGameState.h"

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
//structure gathering info about upgrade possibilites

class ICallback
{
public:
	virtual bool moveHero(int ID, CPath * path, int idtype, int pathType=0)=0;//idtype: 0 - position in vector of heroes (of that player); 1 - ID of hero
															//pathType: 0 - nodes are manifestation pos, 1 - nodes are object pos
	virtual int swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)=0;//swaps creatures between two posiibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)=0;//joins first stack tothe second (creatures must be same type)
	virtual int splitStack(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2, int val)=0;//split creatures from the first stack
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses diven hero; true - successfuly, false - not successfuly
	virtual bool swapArifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual void recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, int stackPos)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1)=0; //if newID==-1 then best possible upgrade will be made
	virtual void endTurn()=0;
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;
	virtual void buyArtifact(const CGHeroInstance *hero, int aid)=0; //used to buy artifacts in towns (including spell book in the guild and war machines in blacksmith)
	virtual void trade(int mode, int id1, int id2, int val1)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce
	virtual void setFormation(const CGHeroInstance * hero, bool tight)=0;

//get info
	virtual bool verifyPath(CPath * path, bool blockSea)const =0;
	virtual int getDate(int mode=0)const =0; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	virtual std::vector< std::vector< std::vector<unsigned char> > > & getVisibilityMap()const =0; //returns visibility map (TODO: make it const)
	virtual const CGHeroInstance * getHeroInfo(int val, int mode=2)const =0; //mode = 0 -> val = serial; mode = 1 -> val = ID
	virtual int getResourceAmount(int type)const =0;
	virtual int howManyHeroes()const =0;
	virtual const CGTownInstance * getTownInfo(int val, bool mode)const =0; //mode = 0 -> val = serial; mode = 1 -> val = ID
	virtual int howManyTowns()const =0;
	virtual std::vector < std::string > getObjDescriptions(int3 pos)const =0; //returns descriptions of objects at pos in order from the lowest to the highest
	virtual std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true)const =0;
	virtual bool isVisible(int3 pos)const =0;
	virtual int getMyColor()const =0;
	virtual int getMySerial()const =0;
	virtual int getHeroSerial(const CGHeroInstance * hero)const =0;
	virtual const CCreatureSet* getGarrison(const CGObjectInstance *obj)const =0;
	virtual UpgradeInfo getUpgradeInfo(const CArmedInstance *obj, int stackPos)const =0;
	virtual const StartInfo * getStartInfo()const =0;
	virtual std::vector < const CGObjectInstance * > getBlockingObjs(int3 pos)const =0;
	virtual std::vector < const CGObjectInstance * > getVisitableObjs(int3 pos)const =0;
	virtual void getMarketOffer(int t1, int t2, int &give, int &rec, int mode=0)const =0;
	virtual std::vector < const CGObjectInstance * > getFlaggableObjects(int3 pos) const =0;
	virtual int3 getMapSize() const =0; //returns size of map - z is 1 for one - level map and 2 for two level map

//battle
	virtual int battleGetBattlefieldType()=0; //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	virtual int battleGetObstaclesAtTile(int tile)=0; //returns bitfield
	virtual int battleGetStack(int pos)=0; //returns ID of stack on the tile
	virtual CStack * battleGetStackByID(int ID)=0; //returns stack info by given ID
	virtual CStack * battleGetStackByPos(int pos)=0; //returns stack info by given pos
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
	HeroMoveDetails(){};
	HeroMoveDetails(int3 Src, int3 Dst, CGHeroInstance*Ho);
	int3 src, dst; //source and destination points
	CGHeroInstance * ho; //object instance of this hero
	int owner, style; //style: 0 - normal move, 1 - teleport, 2 - instant jump
	bool successful;
};

class CCallback : public ICallback
{
private:
	CCallback(CGameState * GS, int Player, CClient *C):gs(GS),player(Player),cl(C){};
	CGameState * gs;
	CClient *cl;
	bool isVisible(int3 pos, int Player) const;
	bool isVisible(CGObjectInstance *obj, int Player) const;

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
	bool swapArifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2);
	bool buildBuilding(const CGTownInstance *town, si32 buildingID);
	void recruitCreatures(const CGObjectInstance *obj, ui32 ID, ui32 amount);
	bool dismissCreature(const CArmedInstance *obj, int stackPos);
	bool upgradeCreature(const CArmedInstance *obj, int stackPos, int newID=-1);
	void endTurn();
	void swapGarrisonHero(const CGTownInstance *town);
	void buyArtifact(const CGHeroInstance *hero, int aid);
	void trade(int mode, int id1, int id2, int val1);
	void setFormation(const CGHeroInstance * hero, bool tight);

//get info
	bool verifyPath(CPath * path, bool blockSea) const;
	int getDate(int mode=0) const; //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	std::vector< std::vector< std::vector<unsigned char> > > & getVisibilityMap() const; //returns visibility map (TODO: make it const)
	const CGHeroInstance * getHeroInfo(int val, int mode=2) const; //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type) const;
	std::vector<si32> getResourceAmount() const;
	int howManyHeroes() const;
	const CGTownInstance * getTownInfo(int val, bool mode) const; //mode = 0 -> val = serial; mode = 1 -> val = ID
	std::vector < const CGTownInstance *> getTownsInfo(bool onlyOur=true) const;
	int howManyTowns()const;
	std::vector < std::string > getObjDescriptions(int3 pos) const; //returns descriptions of objects at pos in order from the lowest to the highest
	std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true) const;
	bool isVisible(int3 pos) const;
	int getMyColor() const;
	int getHeroSerial(const CGHeroInstance * hero) const;
	int getMySerial() const;
	const CCreatureSet* getGarrison(const CGObjectInstance *obj) const;
	UpgradeInfo getUpgradeInfo(const CArmedInstance *obj, int stackPos) const;
	const StartInfo * getStartInfo() const;
	std::vector < const CGObjectInstance * > getBlockingObjs(int3 pos) const;
	std::vector < const CGObjectInstance * > getVisitableObjs(int3 pos) const;
	void getMarketOffer(int t1, int t2, int &give, int &rec, int mode=0) const;
	std::vector < const CGObjectInstance * > getFlaggableObjects(int3 pos) const;
	int3 getMapSize() const; //returns size of map - z is 1 for one - level map and 2 for two level map

	//battle
	int battleGetBattlefieldType(); //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	int battleGetObstaclesAtTile(int tile); //returns bitfield
	int battleGetStack(int pos); //returns ID of stack on the tile
	CStack * battleGetStackByID(int ID); //returns stack info by given ID
	CStack * battleGetStackByPos(int pos); //returns stack info by given pos
	int battleGetPos(int stack); //returns position (tile ID) of stack
	//int battleMakeAction(BattleAction* action);//perform action with an active stack (or custom action)
	std::map<int, CStack> battleGetStacks(); //returns stacks on battlefield
	CCreature battleGetCreature(int number); //returns type of creature by given number of stack
	//bool battleMoveCreature(int ID, int dest); //moves creature with id ID to dest if possible
	std::vector<int> battleGetAvailableHexes(int ID); //reutrns numbers of hexes reachable by creature with id ID
	bool battleIsStackMine(int ID); //returns true if stack with id ID belongs to caller
	bool battleCanShoot(int ID, int dest); //returns true if unit with id ID can shoot to dest



//friends
	friend class CClient;
#ifndef __GNUC__
	friend int _tmain(int argc, _TCHAR* argv[]);
#else
	friend int main(int argc, char** argv);
#endif
};
#endif //CCALLBACK_H
