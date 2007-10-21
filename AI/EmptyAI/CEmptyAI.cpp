#include "CEmptyAI.h"
#include <iostream>
void CEmptyAI::init(CCallback * CB)
{
	cb = CB;
	human=false;
	playerID=-1;
	serialID=-1;
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