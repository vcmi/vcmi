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
struct TerrainTile;
//structure gathering info about upgrade possibilites

class ICallback
{
public:
	virtual bool moveHero(const CGHeroInstance *h, int3 dst) const =0; //dst must be free, neighbouring tile (this function can move hero only by one tile)
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
	virtual void setSelection(const CArmedInstance * obj)=0;
	virtual void recruitHero(const CGTownInstance *town, const CGHeroInstance *hero)=0;

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
	virtual std::vector<const CGHeroInstance *> getAvailableHeroes(const CGTownInstance * town) const =0; //heroes that can be recruited
	virtual const TerrainTile * getTileInfo(int3 tile) = 0;
	virtual int canBuildStructure(const CGTownInstance *t, int ID) =0;//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

//battle
	virtual int battleGetBattlefieldType()=0; //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	virtual int battleGetObstaclesAtTile(int tile)=0; //returns bitfield
	virtual std::vector<CObstacleInstance> battleGetAllObstacles()=0; //returns all obstacles on the battlefield
	virtual int battleGetStack(int pos)=0; //returns ID of stack on the tile
	virtual CStack * battleGetStackByID(int ID)=0; //returns stack info by given ID
	virtual CStack * battleGetStackByPos(int pos)=0; //returns stack info by given pos
	virtual int battleGetPos(int stack)=0; //returns position (tile ID) of stack
	virtual int battleMakeAction(BattleAction* action)=0;//for casting spells by hero - DO NOT use it for moving active stack
	virtual std::map<int, CStack> battleGetStacks()=0; //returns stacks on battlefield
	virtual std::vector<CStack> battleGetStackQueue()=0; //returns vector of stack in order of their move sequence
	virtual CCreature battleGetCreature(int number)=0; //returns type of creature by given number of stack
	//virtual bool battleMoveCreature(int ID, int dest)=0; //moves creature with id ID to dest if possible
	virtual std::vector<int> battleGetAvailableHexes(int ID, bool addOccupiable)=0; //reutrns numbers of hexes reachable by creature with id ID
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
	CCallback(CGameState * GS, int Player, CClient *C):gs(GS), cl(C), player(Player){};
	CGameState * gs;
	CClient *cl;
	bool isVisible(int3 pos, int Player) const;
	bool isVisible(CGObjectInstance *obj, int Player) const;

protected:
	int player;

public:
//commands
	bool moveHero(const CGHeroInstance *h, int3 dst) const; //dst must be free, neighbouring tile (this function can move hero only by one tile)
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
	void setSelection(const CArmedInstance * obj);
	void recruitHero(const CGTownInstance *town, const CGHeroInstance *hero);

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
	std::vector<const CGHeroInstance *> getAvailableHeroes(const CGTownInstance * town) const; //heroes that can be recruited
	const TerrainTile * getTileInfo(int3 tile);
	int canBuildStructure(const CGTownInstance *t, int ID);//// 0 - no more than one capitol, 1 - lack of water, 2 - forbidden, 3 - Add another level to Mage Guild, 4 - already built, 5 - cannot build, 6 - cannot afford, 7 - build, 8 - lack of requirements

	//battle
	int battleGetBattlefieldType(); //   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines   8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields   15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog   21. "favourable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	int battleGetObstaclesAtTile(int tile); //returns bitfield
	std::vector<CObstacleInstance> battleGetAllObstacles(); //returns all obstacles on the battlefield
	int battleGetStack(int pos); //returns ID of stack on the tile
	CStack * battleGetStackByID(int ID); //returns stack info by given ID
	CStack * battleGetStackByPos(int pos); //returns stack info by given pos
	int battleGetPos(int stack); //returns position (tile ID) of stack
	int battleMakeAction(BattleAction* action);//for casting spells by hero - DO NOT use it for moving active stack
	std::map<int, CStack> battleGetStacks(); //returns stacks on battlefield
	std::vector<CStack> battleGetStackQueue(); //returns vector of stack in order of their move sequence
	CCreature battleGetCreature(int number); //returns type of creature by given number of stack
	std::vector<int> battleGetAvailableHexes(int ID, bool addOccupiable); //reutrns numbers of hexes reachable by creature with id ID
	bool battleIsStackMine(int ID); //returns true if stack with id ID belongs to caller
	bool battleCanShoot(int ID, int dest); //returns true if unit with id ID can shoot to dest


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
