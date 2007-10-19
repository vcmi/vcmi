#ifndef CCALLBACK_H
#define CCALLBACK_H

#include "mapHandler.h"

class CGameState;
class CHeroInstance;
class CTownInstance;
class CPath;
class CObjectInstance;

struct HeroMoveDetails
{
	int3 src, dst; //source and destination points
	CObjectInstance * ho; //object instance of this hero
	int owner;
};

class CCallback 
{
private:
	void newTurn();
	CCallback(CGameState * GS, int Player):gs(GS),player(Player){};
	CGameState * gs;
	int lowestSpeed(CHeroInstance * chi); //speed of the slowest stack
	int valMovePoints(CHeroInstance * chi); 
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
	const CHeroInstance * getHeroInfo(int player, int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type);
	int howManyHeroes();
	const CTownInstance * getTownInfo(int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int howManyTowns();
	std::vector < std::string > getObjDescriptions(int3 pos); //returns descriptions of objects at pos in order from the lowest to the highest
	std::vector < const CHeroInstance *> * getHeroesInfo(bool onlyOur=true);
	bool isVisible(int3 pos);

//friends
	friend int _tmain(int argc, _TCHAR* argv[]);
};

#endif //CCALLBACK_H