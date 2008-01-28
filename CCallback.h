#ifndef CCALLBACK_H
#define CCALLBACK_H

#include "mapHandler.h"
#include "tchar.h"
class CGameState;
class CHeroInstance;
class CTownInstance;
class CPath;
class CGObjectInstance;
class SComponent;
class IChosen;
class CSelectableComponent;
typedef struct lua_State lua_State;

class ICallback
{	
public:
	virtual bool moveHero(int ID, CPath * path, int idtype, int pathType=0)=0;//idtype: 0 - position in vector of heroes (of that player); 1 - ID of hero 
															//pathType: 0 - nodes are manifestation pos, 1 - nodes are object pos

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
	virtual int swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2)=0;//swaps creatures between two posiibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses diven hero; true - successfuly, false - not successfuly
	virtual const CCreatureSet* getGarrison(const CGObjectInstance *obj)=0;
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

//get info
	bool verifyPath(CPath * path, bool blockSea);
	int getDate(int mode=0); //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	PseudoV< PseudoV< PseudoV<unsigned char> > > & getVisibilityMap(); //returns visibility map (TODO: make it const)
	const CGHeroInstance * getHeroInfo(int player, int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type);
	int howManyHeroes();
	const CGTownInstance * getTownInfo(int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int howManyTowns();
	std::vector < std::string > getObjDescriptions(int3 pos); //returns descriptions of objects at pos in order from the lowest to the highest
	std::vector < const CGHeroInstance *> getHeroesInfo(bool onlyOur=true);
	bool isVisible(int3 pos);
	int getMyColor();
	int getHeroSerial(const CGHeroInstance * hero);
	int getMySerial();
	int swapCreatures(const CGObjectInstance *s1, const CGObjectInstance *s2, int p1, int p2);
	bool dismissHero(const CGHeroInstance * hero);
	const CCreatureSet* getGarrison(const CGObjectInstance *obj);

//friends
	friend int _tmain(int argc, _TCHAR* argv[]);
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