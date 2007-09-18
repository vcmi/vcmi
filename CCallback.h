#ifndef CCALLBACK_H
#define CCALLBACK_H
class CGameState;
class CHeroInstance;
class CTownInstance;
struct HeroMoveDetails
{
	int3 src, dst; //source and destination points
	int heroID; //which hero
	int owner;
};
class CCallback 
{
private:
	void newTurn();

protected:
	CGameState * gs;

public:
	CCallback(CGameState * GS):gs(GS){};
	bool moveHero(int ID, int3 destPoint);

	int howManyTowns();
	const CTownInstance * getTownInfo(int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID

	int howManyHeroes(int player);
	const CHeroInstance * getHeroInfo(int player, int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type);
	int getDate(int mode=0); //mode=0 - total days in game, mode=1 - day of week, mode=2 - current week, mode=3 - current month
};

#endif //CCALLBACK_H