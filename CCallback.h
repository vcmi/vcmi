#ifndef CCALLBACK_H
#define CCALLBACK_H
class CGameState;
class CCallback 
{
protected:
	CGameState * gs;
public:
	bool moveHero(int ID, int3 destPoint);
};

#endif //CCALLBACK_H