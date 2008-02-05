#include "CEmptyAI.h"
#include <iostream>
void CEmptyAI::init(ICallback * CB)
{
	cb = CB;
	human=false;
	playerID=cb->getMyColor();
	serialID=cb->getMySerial();
	std::cout << "EmptyAI initialized." << std::endl;
}
void CEmptyAI::yourTurn()
{
}
void CEmptyAI::heroKilled(const CGHeroInstance *)
{
}
void CEmptyAI::heroCreated(const CGHeroInstance *)
{
}
void CEmptyAI::heroMoved(const HeroMoveDetails &)
{
}