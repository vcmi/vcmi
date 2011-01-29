#include "CEmptyAI.h"
#include <iostream>
void CEmptyAI::init(ICallback * CB)
{
	cb = CB;
	human=false;
	playerID=cb->getMyColor();
	std::cout << "EmptyAI initialized." << std::endl;
}
void CEmptyAI::yourTurn()
{
	cb->endTurn();
}
void CEmptyAI::heroKilled(const CGHeroInstance *)
{
}
void CEmptyAI::heroCreated(const CGHeroInstance *)
{
}
void CEmptyAI::heroMoved(const TryMoveHero& TMH)
{
}
void CEmptyAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand()%skills.size());
}
