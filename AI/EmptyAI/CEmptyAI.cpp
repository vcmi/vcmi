#include "StdInc.h"
#include "CEmptyAI.h"

void CEmptyAI::init(CCallback * CB)
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
void CEmptyAI::heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback)
{
	callback(rand()%skills.size());
}

void CEmptyAI::commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, boost::function<void(ui32)> &callback)
{
	callback(0);
}

void CEmptyAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel)
{
	cb->selectionMade(0, askID);
}

void CEmptyAI::showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd)
{
	onEnd();
}