#include "CEmptyAI.h"
#include <iostream>
void CEmptyAI::init(CCallback * CB)
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
void CEmptyAI::heroKilled(const CHeroInstance *)
{
}
void CEmptyAI::heroCreated(const CHeroInstance *)
{
}
void CEmptyAI::heroMoved(const HeroMoveDetails &)
{
}