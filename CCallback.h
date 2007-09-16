#ifndef CCALLBACK_H
#define CCALLBACK_H
class CGameState;
class CHeroInstance;
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

	int howManyHeroes(int player);
	const CHeroInstance * getHeroInfo(int player, int val, bool mode); //mode = 0 -> val = serial; mode = 1 -> val = ID
	int getResourceAmount(int type);
};

#endif //CCALLBACK_H