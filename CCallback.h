#ifndef CCALLBACK_H
#define CCALLBACK_H

#include "mapHandler.h"
#include "tchar.h"
class CGameState;
class CHeroInstance;
class CTownInstance;
class CPath;
class CGObjectInstance;
struct SComponent;
typedef struct lua_State lua_State;
struct HeroMoveDetails
{
	int3 src, dst; //source and destination points
	CGHeroInstance * ho; //object instance of this hero
	int owner;
};

class CCallback 
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
	std::vector < const CGHeroInstance *> * getHeroesInfo(bool onlyOur=true);
	bool isVisible(int3 pos);

//friends
	friend int _tmain(int argc, _TCHAR* argv[]);
};
class CScriptCallback
{
public:
	CGameState * gs;

	static int3 getPos(CGObjectInstance * ob);
	static void changePrimSkill(int ID, int which, int val);
	void showInfoDialog(int player, std::string text, std::vector<SComponent*> * components);
	int getHeroOwner(int heroID);
	int getSelectedHero();
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