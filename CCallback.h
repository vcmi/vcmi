#ifndef CCALLBACK_H
#define CCALLBACK_H
class CGameState;
class CHeroInstance;
class CTownInstance;
struct HeroMoveDetails
{
	int3 src, dst; //source and destination points
	int heroID; //position in vector 
	int owner;
};
class CCallback 
{
private:
	int player;
	void newTurn();
	CCallback(CGameState * GS, int Player):gs(GS),player(Player){};

protected:
	CGameState * gs;

public:
	bool moveHero(int ID, int3 destPoint, int idtype=0);//idtype: 0-position in vector; 1-ID of hero 

	int howManyTowns();
	const CTownInstance * getTownInfo(int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID

	int howManyHeroes(int player);
	const CHeroInstance * getHeroInfo(int player, int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type);
	int getDate(int mode=0); //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
	
	friend int _tmain(int argc, _TCHAR* argv[]);
};

#endif //CCALLBACK_H