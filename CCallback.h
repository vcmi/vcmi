#ifndef CCALLBACK_H
#define CCALLBACK_H
class CGameState;
struct HeroMoveDetails
{
	int3 src, dst; //source and destination points
	int heroID; //which hero
	int owner;
};
class CCallback 
{
protected:
	CGameState * gs;

public:
	bool moveHero(int ID, int3 destPoint);
};

#endif //CCALLBACK_H