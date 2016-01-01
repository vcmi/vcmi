#include "StdInc.h"
#include "CEmptyAI.h"

#include "../../lib/CRandomGenerator.h"

void CEmptyAI::init(std::shared_ptr<CCallback> CB)
{
	cb = CB;
	human=false;
	playerID = *cb->getMyColor();
	//logAi->infoStream() << "EmptyAI initialized.";
}
void CEmptyAI::yourTurn()
{
	cb->endTurn();
}

void CEmptyAI::heroGotLevel(const CGHeroInstance *hero, PrimarySkill::PrimarySkill pskill, std::vector<SecondarySkill> &skills, QueryID queryID)
{
	cb->selectionMade(CRandomGenerator::getDefault().nextInt(skills.size() - 1), queryID);
}

void CEmptyAI::commanderGotLevel(const CCommanderInstance * commander, std::vector<ui32> skills, QueryID queryID)
{
	cb->selectionMade(CRandomGenerator::getDefault().nextInt(skills.size() - 1), queryID);
}

void CEmptyAI::showBlockingDialog(const std::string &text, const std::vector<Component> &components, QueryID askID, const int soundID, bool selection, bool cancel)
{
	cb->selectionMade(0, askID);
}

void CEmptyAI::showTeleportDialog(TeleportChannelID channel, TTeleportExitsList exits, bool impassable, QueryID askID)
{
	cb->selectionMade(0, askID);
}

void CEmptyAI::showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, QueryID queryID)
{
	cb->selectionMade(0, queryID);
}
